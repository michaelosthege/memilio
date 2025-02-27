/*
* Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
*
* Authors: Daniel Abele
*
* Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "memilio/compartments/parameter_studies.h"
#include "memilio/epidemiology/regions.h"
#include "memilio/io/epi_data.h"
#include "memilio/io/result_io.h"
#include "memilio/io/mobility_io.h"
#include "ode_secir/parameters_io.h"
#include "ode_secir/parameter_space.h"
#include "boost/filesystem.hpp"
#include <cstdio>
#include <iomanip>

namespace fs = boost::filesystem;

/**
 * indices of contact matrix corresponding to locations where contacts occur.
 */
enum class ContactLocation
{
    Home = 0,
    School,
    Work,
    Other,
    Count,
};

/**
 * different types of NPI, used as DampingType.
 */
enum class Intervention
{
    Home,
    SchoolClosure,
    HomeOffice,
    GatheringBanFacilitiesClosure,
    PhysicalDistanceAndMasks,
    SeniorAwareness,
};

/**
 * different level of NPI, used as DampingLevel.
 */
enum class InterventionLevel
{
    Main,
    PhysicalDistanceAndMasks,
    SeniorAwareness,
    Holidays,
};

/**
 * Set a value and distribution of an UncertainValue.
 * Assigns average of min and max as a value and UNIFORM(min, max) as a distribution.
 * @param p uncertain value to set.
 * @param min minimum of distribution.
 * @param max minimum of distribution.
 */
void assign_uniform_distribution(mio::UncertainValue& p, double min, double max)
{
    p = mio::UncertainValue(0.5 * (max + min));
    p.set_distribution(mio::ParameterDistributionUniform(min, max));
}

/**
 * Set a value and distribution of an array of UncertainValues.
 * Assigns average of min[i] and max[i] as a value and UNIFORM(min[i], max[i]) as a distribution for
 * each element i of the array.
 * @param array array of UncertainValues to set.
 * @param min minimum of distribution for each element of array.
 * @param max minimum of distribution for each element of array.
 */
template <size_t N>
void array_assign_uniform_distribution(mio::CustomIndexArray<mio::UncertainValue, mio::AgeGroup>& array,
                                       const double (&min)[N], const double (&max)[N])
{
    assert(N == array.numel());
    for (auto i = mio::AgeGroup(0); i < mio::AgeGroup(N); ++i) {
        assign_uniform_distribution(array[i], min[size_t(i)], max[size_t(i)]);
    }
}

/**
 * Set a value and distribution of an array of UncertainValues.
 * Assigns average of min and max as a value and UNIFORM(min, max) as a distribution to every element of the array.
 * @param array array of UncertainValues to set.
 * @param min minimum of distribution.
 * @param max minimum of distribution.
 */
void array_assign_uniform_distribution(mio::CustomIndexArray<mio::UncertainValue, mio::AgeGroup>& array, double min,
                                       double max)
{
    for (auto i = mio::AgeGroup(0); i < array.size<mio::AgeGroup>(); ++i) {
        assign_uniform_distribution(array[i], min, max);
    }
}

/**
 * Set epidemiological parameters of Sars-CoV-2 for a immune-naive
 * population and wild type variant.
 * @param params Object that the parameters will be added to.
 * @returns Currently generates no errors.
 */
mio::IOResult<void> set_covid_parameters(mio::osecir::Parameters& params)
{
    //times
    const double incubationTime            = 5.2;
    const double serialIntervalMin         = 0.5 * 2.67 + 0.5 * 5.2;
    const double serialIntervalMax         = 0.5 * 4.00 + 0.5 * 5.2;
    const double timeInfectedSymptomsMin[] = {5.6255, 5.6255, 5.6646, 5.5631, 5.501, 5.465};
    const double timeInfectedSymptomsMax[] = {8.427, 8.427, 8.4684, 8.3139, 8.169, 8.085};
    const double timeInfectedSevereMin[]   = {3.925, 3.925, 4.85, 6.4, 7.2, 9.};
    const double timeInfectedSevereMax[]   = {6.075, 6.075, 7., 8.7, 9.8, 13.};
    const double timeInfectedCriticalMin[] = {4.95, 4.95, 4.86, 14.14, 14.4, 10.};
    const double timeInfectedCriticalMax[] = {8.95, 8.95, 8.86, 20.58, 19.8, 13.2};

    array_assign_uniform_distribution(params.get<mio::osecir::IncubationTime>(), incubationTime, incubationTime);
    array_assign_uniform_distribution(params.get<mio::osecir::SerialInterval>(), serialIntervalMin, serialIntervalMax);
    array_assign_uniform_distribution(params.get<mio::osecir::TimeInfectedSymptoms>(), timeInfectedSymptomsMin,
                                      timeInfectedSymptomsMax);
    array_assign_uniform_distribution(params.get<mio::osecir::TimeInfectedSevere>(), timeInfectedSevereMin,
                                      timeInfectedSevereMax);
    array_assign_uniform_distribution(params.get<mio::osecir::TimeInfectedCritical>(), timeInfectedCriticalMin,
                                      timeInfectedCriticalMax);

    //probabilities
    const double transmissionProbabilityOnContactMin[] = {0.02, 0.05, 0.05, 0.05, 0.08, 0.15};
    const double transmissionProbabilityOnContactMax[] = {0.04, 0.07, 0.07, 0.07, 0.10, 0.20};
    const double relativeTransmissionNoSymptomsMin     = 1;
    const double relativeTransmissionNoSymptomsMax     = 1;
    // The precise value between Risk* (situation under control) and MaxRisk* (situation not under control)
    // depends on incidence and test and trace capacity
    const double riskOfInfectionFromSymptomaticMin    = 0.1;
    const double riskOfInfectionFromSymptomaticMax    = 0.3;
    const double maxRiskOfInfectionFromSymptomaticMin = 0.3;
    const double maxRiskOfInfectionFromSymptomaticMax = 0.5;
    const double recoveredPerInfectedNoSymptomsMin[]  = {0.2, 0.2, 0.15, 0.15, 0.15, 0.15};
    const double recoveredPerInfectedNoSymptomsMax[]  = {0.3, 0.3, 0.25, 0.25, 0.25, 0.25};
    const double severePerInfectedSymptomsMin[]       = {0.006, 0.006, 0.015, 0.049, 0.15, 0.20};
    const double severePerInfectedSymptomsMax[]       = {0.009, 0.009, 0.023, 0.074, 0.18, 0.25};
    const double criticalPerSevereMin[]               = {0.05, 0.05, 0.05, 0.10, 0.25, 0.35};
    const double criticalPerSevereMax[]               = {0.10, 0.10, 0.10, 0.20, 0.35, 0.45};
    const double deathsPerCriticalMin[]               = {0.00, 0.00, 0.10, 0.10, 0.30, 0.5};
    const double deathsPerCriticalMax[]               = {0.10, 0.10, 0.18, 0.18, 0.50, 0.7};

    array_assign_uniform_distribution(params.get<mio::osecir::TransmissionProbabilityOnContact>(),
                                      transmissionProbabilityOnContactMin, transmissionProbabilityOnContactMax);
    array_assign_uniform_distribution(params.get<mio::osecir::RelativeTransmissionNoSymptoms>(),
                                      relativeTransmissionNoSymptomsMin, relativeTransmissionNoSymptomsMax);
    array_assign_uniform_distribution(params.get<mio::osecir::RiskOfInfectionFromSymptomatic>(),
                                      riskOfInfectionFromSymptomaticMin, riskOfInfectionFromSymptomaticMax);
    array_assign_uniform_distribution(params.get<mio::osecir::MaxRiskOfInfectionFromSymptomatic>(),
                                      maxRiskOfInfectionFromSymptomaticMin, maxRiskOfInfectionFromSymptomaticMax);
    array_assign_uniform_distribution(params.get<mio::osecir::RecoveredPerInfectedNoSymptoms>(),
                                      recoveredPerInfectedNoSymptomsMin, recoveredPerInfectedNoSymptomsMax);
    array_assign_uniform_distribution(params.get<mio::osecir::SeverePerInfectedSymptoms>(),
                                      severePerInfectedSymptomsMin, severePerInfectedSymptomsMax);
    array_assign_uniform_distribution(params.get<mio::osecir::CriticalPerSevere>(), criticalPerSevereMin,
                                      criticalPerSevereMax);
    array_assign_uniform_distribution(params.get<mio::osecir::DeathsPerCritical>(), deathsPerCriticalMin,
                                      deathsPerCriticalMax);

    //sasonality
    const double seasonality_min = 0.1;
    const double seasonality_max = 0.3;

    assign_uniform_distribution(params.get<mio::osecir::Seasonality>(), seasonality_min, seasonality_max);

    return mio::success();
}

static const std::map<ContactLocation, std::string> contact_locations = {{ContactLocation::Home, "home"},
                                                                         {ContactLocation::School, "school_pf_eig"},
                                                                         {ContactLocation::Work, "work"},
                                                                         {ContactLocation::Other, "other"}};

/**
 * Set contact matrices.
 * Reads contact matrices from files in the data directory.
 * @param data_dir data directory.
 * @param params Object that the contact matrices will be added to.
 * @returns any io errors that happen during reading of the files.
 */
mio::IOResult<void> set_contact_matrices(const fs::path& data_dir, mio::osecir::Parameters& params)
{
    //TODO: io error handling
    auto contact_matrices = mio::ContactMatrixGroup(contact_locations.size(), size_t(params.get_num_groups()));
    for (auto&& contact_location : contact_locations) {
        BOOST_OUTCOME_TRY(baseline,
                          mio::read_mobility_plain(
                              (data_dir / "contacts" / ("baseline_" + contact_location.second + ".txt")).string()));
        BOOST_OUTCOME_TRY(minimum,
                          mio::read_mobility_plain(
                              (data_dir / "contacts" / ("minimum_" + contact_location.second + ".txt")).string()));
        contact_matrices[size_t(contact_location.first)].get_baseline() = baseline;
        contact_matrices[size_t(contact_location.first)].get_minimum()  = minimum;
    }
    params.get<mio::osecir::ContactPatterns>() = mio::UncertainContactMatrix(contact_matrices);

    return mio::success();
}

/**
 * Set NPIs.
 * @param start_date start date of the simulation.
 * @param end_date end date of the simulation.
 * @param params Object that the NPIs will be added to.
 * @returns Currently generates no errors.
 */
mio::IOResult<void> set_npis(mio::Date start_date, mio::Date end_date, mio::osecir::Parameters& params)
{
    auto& contacts         = params.get<mio::osecir::ContactPatterns>();
    auto& contact_dampings = contacts.get_dampings();

    //weights for age groups affected by an NPI
    auto group_weights_all     = Eigen::VectorXd::Constant(size_t(params.get_num_groups()), 1.0);
    auto group_weights_seniors = Eigen::VectorXd::NullaryExpr(size_t(params.get_num_groups()), [](auto&& i) {
        return i == 5 ? 1.0 : i == 4 ? 0.5 : 0.0; //65-80 only partially
    });

    //helper functions that create dampings for specific NPIs
    auto contacts_at_home = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::Main)),
                                    mio::DampingType(int(Intervention::Home)), t, {size_t(ContactLocation::Home)},
                                    group_weights_all);
    };
    auto school_closure = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::Main)),
                                    mio::DampingType(int(Intervention::SchoolClosure)), t,
                                    {size_t(ContactLocation::School)}, group_weights_all);
    };
    auto home_office = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::Main)),
                                    mio::DampingType(int(Intervention::HomeOffice)), t, {size_t(ContactLocation::Work)},
                                    group_weights_all);
    };
    auto social_events = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::Main)),
                                    mio::DampingType(int(Intervention::GatheringBanFacilitiesClosure)), t,
                                    {size_t(ContactLocation::Other)}, group_weights_all);
    };
    auto social_events_work = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::Main)),
                                    mio::DampingType(int(Intervention::GatheringBanFacilitiesClosure)), t,
                                    {size_t(ContactLocation::Work)}, group_weights_all);
    };
    auto physical_distancing_home_school = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::PhysicalDistanceAndMasks)),
                                    mio::DampingType(int(Intervention::PhysicalDistanceAndMasks)), t,
                                    {size_t(ContactLocation::Home), size_t(ContactLocation::School)},
                                    group_weights_all);
    };
    auto physical_distancing_work_other = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::PhysicalDistanceAndMasks)),
                                    mio::DampingType(int(Intervention::PhysicalDistanceAndMasks)), t,
                                    {size_t(ContactLocation::Work), size_t(ContactLocation::Other)}, group_weights_all);
    };
    auto senior_awareness = [=](auto t, auto min, auto max) {
        auto v = mio::UncertainValue();
        assign_uniform_distribution(v, min, max);
        return mio::DampingSampling(v, mio::DampingLevel(int(InterventionLevel::SeniorAwareness)),
                                    mio::DampingType(int(Intervention::SeniorAwareness)), t,
                                    {size_t(ContactLocation::Home), size_t(ContactLocation::Other)},
                                    group_weights_seniors);
    };

    //SPRING 2020 LOCKDOWN SCENARIO
    auto start_spring_date = mio::Date(2020, 3, 18);
    if (start_spring_date < end_date) {
        auto start_spring = mio::SimulationTime(mio::get_offset_in_days(start_spring_date, start_date));
        contact_dampings.push_back(contacts_at_home(start_spring, 0.6, 0.8));
        contact_dampings.push_back(school_closure(start_spring, 1.0, 1.0));
        contact_dampings.push_back(home_office(start_spring, 0.2, 0.3));
        contact_dampings.push_back(social_events(start_spring, 0.6, 0.8));
        contact_dampings.push_back(social_events_work(start_spring, 0.1, 0.2));
        contact_dampings.push_back(physical_distancing_home_school(start_spring, 0.4, 0.6));
        contact_dampings.push_back(physical_distancing_work_other(start_spring, 0.4, 0.6));
        contact_dampings.push_back(senior_awareness(start_spring, 0.0, 0.0));
    }

    // SUMMER 2020 SCENARIO
    auto start_summer_date = mio::Date(2020, 5, 15);
    if (start_summer_date < end_date) {
        auto start_summer       = mio::SimulationTime(mio::get_offset_in_days(start_summer_date, start_date));
        auto school_reopen_time = mio::SimulationTime(mio::get_offset_in_days(mio::Date(2020, 6, 15), start_date));
        contact_dampings.push_back(contacts_at_home(start_summer, 0.0, 0.2));
        contact_dampings.push_back(school_closure(start_summer, 0.5, 0.5)); //schools partially reopened
        contact_dampings.push_back(school_closure(school_reopen_time, 0.0, 0.0)); //school fully reopened
        contact_dampings.push_back(home_office(start_summer, 0.2, 0.3));
        contact_dampings.push_back(social_events(start_summer, 0.0, 0.2));
        contact_dampings.push_back(social_events_work(start_summer, 0.0, 0.05));
        contact_dampings.push_back(physical_distancing_home_school(start_summer, 0.0, 0.2));
        contact_dampings.push_back(physical_distancing_work_other(start_summer, 0.0, 0.2));
        contact_dampings.push_back(senior_awareness(start_summer, 0.0, 0.0));
    }

    //autumn enforced attention
    auto start_autumn_date = mio::Date(2020, 10, 1);
    if (start_autumn_date < end_date) {
        auto start_autumn = mio::SimulationTime(mio::get_offset_in_days(start_autumn_date, start_date));
        contact_dampings.push_back(contacts_at_home(start_autumn, 0.2, 0.4));
        contact_dampings.push_back(physical_distancing_home_school(start_autumn, 0.2, 0.4));
        contact_dampings.push_back(physical_distancing_work_other(start_autumn, 0.2, 0.4));
    }

    //autumn lockdown light
    auto start_autumn_lockdown_date = mio::Date(2020, 11, 1);
    if (start_autumn_lockdown_date < end_date) {
        auto start_autumn_lockdown =
            mio::SimulationTime(mio::get_offset_in_days(start_autumn_lockdown_date, start_date));
        contact_dampings.push_back(contacts_at_home(start_autumn_lockdown, 0.4, 0.6));
        contact_dampings.push_back(school_closure(start_autumn_lockdown, 0.0, 0.0));
        contact_dampings.push_back(home_office(start_autumn_lockdown, 0.2, 0.3));
        contact_dampings.push_back(social_events(start_autumn_lockdown, 0.6, 0.8));
        contact_dampings.push_back(social_events_work(start_autumn_lockdown, 0.0, 0.1));
        contact_dampings.push_back(physical_distancing_home_school(start_autumn_lockdown, 0.2, 0.4));
        contact_dampings.push_back(physical_distancing_work_other(start_autumn_lockdown, 0.4, 0.6));
        contact_dampings.push_back(senior_awareness(start_autumn_lockdown, 0.0, 0.0));
    }

    //winter lockdown
    auto start_winter_lockdown_date = mio::Date(2020, 12, 16);
    if (start_winter_lockdown_date < end_date) {
        double min = 0.6, max = 0.8; //for strictest scenario: 0.8 - 1.0
        auto start_winter_lockdown =
            mio::SimulationTime(mio::get_offset_in_days(start_winter_lockdown_date, start_date));
        contact_dampings.push_back(contacts_at_home(start_winter_lockdown, min, max));
        contact_dampings.push_back(school_closure(start_winter_lockdown, 1.0, 1.0));
        contact_dampings.push_back(home_office(start_winter_lockdown, 0.2, 0.3));
        contact_dampings.push_back(social_events(start_winter_lockdown, min, max));
        contact_dampings.push_back(social_events_work(start_winter_lockdown, 0.1, 0.2));
        contact_dampings.push_back(physical_distancing_home_school(start_winter_lockdown, 0.2, 0.4));
        contact_dampings.push_back(physical_distancing_work_other(start_winter_lockdown, min, max));
        contact_dampings.push_back(senior_awareness(start_winter_lockdown, 0.0, 0.0));

        //relaxing of restrictions over christmas days
        auto xmas_date = mio::Date(2020, 12, 24);
        auto xmas      = mio::SimulationTime(mio::get_offset_in_days(xmas_date, start_date));
        contact_dampings.push_back(contacts_at_home(xmas, 0.0, 0.0));
        contact_dampings.push_back(home_office(xmas, 0.4, 0.5));
        contact_dampings.push_back(social_events(xmas, 0.4, 0.6));
        contact_dampings.push_back(physical_distancing_home_school(xmas, 0.0, 0.0));
        contact_dampings.push_back(physical_distancing_work_other(xmas, 0.4, 0.6));

        // after christmas
        auto after_xmas_date = mio::Date(2020, 12, 27);
        auto after_xmas      = mio::SimulationTime(mio::get_offset_in_days(after_xmas_date, start_date));
        contact_dampings.push_back(contacts_at_home(after_xmas, min, max));
        contact_dampings.push_back(home_office(after_xmas, 0.2, 0.3));
        contact_dampings.push_back(social_events(after_xmas, 0.6, 0.8));
        contact_dampings.push_back(physical_distancing_home_school(after_xmas, 0.2, 0.4));
        contact_dampings.push_back(physical_distancing_work_other(after_xmas, min, max));
    }

    //local dynamic NPIs
    auto& dynamic_npis        = params.get<mio::osecir::DynamicNPIsInfectedSymptoms>();
    auto dynamic_npi_dampings = std::vector<mio::DampingSampling>();
    dynamic_npi_dampings.push_back(
        contacts_at_home(mio::SimulationTime(0), 0.6, 0.8)); // increased from [0.4, 0.6] in Nov
    dynamic_npi_dampings.push_back(school_closure(mio::SimulationTime(0), 0.25, 0.25)); // see paper
    dynamic_npi_dampings.push_back(home_office(mio::SimulationTime(0), 0.2, 0.3)); // ...
    dynamic_npi_dampings.push_back(social_events(mio::SimulationTime(0), 0.6, 0.8));
    dynamic_npi_dampings.push_back(social_events_work(mio::SimulationTime(0), 0.1, 0.2));
    dynamic_npi_dampings.push_back(physical_distancing_home_school(mio::SimulationTime(0), 0.6, 0.8));
    dynamic_npi_dampings.push_back(physical_distancing_work_other(mio::SimulationTime(0), 0.6, 0.8));
    dynamic_npi_dampings.push_back(senior_awareness(mio::SimulationTime(0), 0.0, 0.0));
    dynamic_npis.set_interval(mio::SimulationTime(3.0));
    dynamic_npis.set_duration(mio::SimulationTime(14.0));
    dynamic_npis.set_base_value(100'000);
    dynamic_npis.set_threshold(200.0, dynamic_npi_dampings);

    //school holidays (holiday periods are set per node, see set_nodes)
    auto school_holiday_value = mio::UncertainValue();
    assign_uniform_distribution(school_holiday_value, 1.0, 1.0);
    contacts.get_school_holiday_damping() =
        mio::DampingSampling(school_holiday_value, mio::DampingLevel(int(InterventionLevel::Holidays)),
                             mio::DampingType(int(Intervention::SchoolClosure)), mio::SimulationTime(0.0),
                             {size_t(ContactLocation::School)}, group_weights_all);

    return mio::success();
}

/**
 * Set synthetic population data for testing.
 * Same total populaton but different spread of infection in each county.
 * @param counties parameters for each county.
 */
void set_synthetic_population_data(std::vector<mio::osecir::Model>& counties)
{
    for (size_t county_idx = 0; county_idx < counties.size(); ++county_idx) {
        double nb_total_t0 = 10000, nb_exp_t0 = 2, nb_inf_t0 = 0, nb_car_t0 = 0, nb_hosp_t0 = 0, nb_icu_t0 = 0,
               nb_rec_t0 = 0, nb_dead_t0 = 0;

        nb_exp_t0 = (double)(county_idx % 10 + 1) * 3;

        for (mio::AgeGroup i = 0; i < counties[county_idx].parameters.get_num_groups(); i++) {
            counties[county_idx].populations[{i, mio::osecir::InfectionState::Exposed}]            = nb_exp_t0;
            counties[county_idx].populations[{i, mio::osecir::InfectionState::InfectedNoSymptoms}] = nb_car_t0;
            counties[county_idx].populations[{i, mio::osecir::InfectionState::InfectedSymptoms}]   = nb_inf_t0;
            counties[county_idx].populations[{i, mio::osecir::InfectionState::InfectedSevere}]     = nb_hosp_t0;
            counties[county_idx].populations[{i, mio::osecir::InfectionState::InfectedCritical}]   = nb_icu_t0;
            counties[county_idx].populations[{i, mio::osecir::InfectionState::Recovered}]          = nb_rec_t0;
            counties[county_idx].populations[{i, mio::osecir::InfectionState::Dead}]               = nb_dead_t0;
            counties[county_idx].populations.set_difference_from_group_total<mio::AgeGroup>(
                {i, mio::osecir::InfectionState::Susceptible}, nb_total_t0);
        }
    }
}

/**
 * Adds county nodes to graph.
 * Reads list counties and populations from files in the data directory. 
 * @param params Parameters that are shared between all nodes.
 * @param start_date start date of the simulation.
 * @param end_date end date of the simulation.
 * @param data_dir data directory.
 * @param params_graph graph object that the nodes will be added to.
 * @returns any io errors that happen during reading of the files.
 */
mio::IOResult<void> set_nodes(const mio::osecir::Parameters& params, mio::Date start_date, mio::Date end_date,
                              const fs::path& data_dir,
                              mio::Graph<mio::osecir::Model, mio::MigrationParameters>& params_graph)
{
    namespace de = mio::regions::de;

    BOOST_OUTCOME_TRY(county_ids, mio::get_county_ids((data_dir / "pydata" / "Germany").string()));
    std::vector<mio::osecir::Model> counties(county_ids.size(),
                                             mio::osecir::Model(int(size_t(params.get_num_groups()))));
    for (auto& county : counties) {
        county.parameters = params;
    }
    auto scaling_factor_infected = std::vector<double>(size_t(params.get_num_groups()), 2.5);
    auto scaling_factor_icu      = 1.0;
    BOOST_OUTCOME_TRY(mio::osecir::read_population_data_county(counties, start_date, county_ids,
                                                               scaling_factor_infected, scaling_factor_icu,
                                                               (data_dir / "pydata" / "Germany").string()));
    // set_synthetic_population_data(counties);

    for (size_t county_idx = 0; county_idx < counties.size(); ++county_idx) {

        //local parameters
        auto tnt_capacity = counties[county_idx].populations.get_total() * 7.5 / 100000.;
        assign_uniform_distribution(counties[county_idx].parameters.get<mio::osecir::TestAndTraceCapacity>(),
                                    0.8 * tnt_capacity, 1.2 * tnt_capacity);

        //holiday periods (damping set globally, see set_npis)
        auto holiday_periods =
            de::get_holidays(de::get_state_id(de::CountyId(county_ids[county_idx])), start_date, end_date);
        auto& contacts = counties[county_idx].parameters.get<mio::osecir::ContactPatterns>();
        contacts.get_school_holidays() =
            std::vector<std::pair<mio::SimulationTime, mio::SimulationTime>>(holiday_periods.size());
        std::transform(
            holiday_periods.begin(), holiday_periods.end(), contacts.get_school_holidays().begin(), [=](auto& period) {
                return std::make_pair(mio::SimulationTime(mio::get_offset_in_days(period.first, start_date)),
                                      mio::SimulationTime(mio::get_offset_in_days(period.second, start_date)));
            });

        //uncertainty in populations
        //TODO: do we need uncertainty in age groups as well?
        for (auto i = mio::AgeGroup(0); i < params.get_num_groups(); i++) {
            for (auto j = mio::Index<mio::osecir::InfectionState>(0); j < mio::osecir::InfectionState::Count; ++j) {
                auto& compartment_value = counties[county_idx].populations[{i, j}];
                assign_uniform_distribution(compartment_value, 0.9 * double(compartment_value),
                                            1.1 * double(compartment_value));
            }
        }

        params_graph.add_node(county_ids[county_idx], counties[county_idx]);
    }
    return mio::success();
}

/**
 * Adds edges to graph.
 * Edges represent commuting and other mobility between counties.
 * Reads mobility from files in the data directory.
 * @param data_dir data directory.
 * @param params_graph graph object that the nodes will be added to.
 * @returns any io errors that happen during reading of the files.
 */
mio::IOResult<void> set_edges(const fs::path& data_dir,
                              mio::Graph<mio::osecir::Model, mio::MigrationParameters>& params_graph)
{
    // mobility between nodes
    BOOST_OUTCOME_TRY(mobility_data_commuter,
                      mio::read_mobility_plain((data_dir / "mobility" / "commuter_migration_scaled.txt").string()));
    BOOST_OUTCOME_TRY(mobility_data_twitter,
                      mio::read_mobility_plain((data_dir / "mobility" / "twitter_scaled_1252.txt").string()));
    if (size_t(mobility_data_commuter.rows()) != params_graph.nodes().size() ||
        size_t(mobility_data_commuter.cols()) != params_graph.nodes().size() ||
        size_t(mobility_data_twitter.rows()) != params_graph.nodes().size() ||
        size_t(mobility_data_twitter.cols()) != params_graph.nodes().size()) {
        return mio::failure(mio::StatusCode::InvalidValue, "Mobility matrices not the correct size.");
    }

    auto migrating_compartments = {mio::osecir::InfectionState::Susceptible, mio::osecir::InfectionState::Exposed,
                                   mio::osecir::InfectionState::InfectedNoSymptoms,
                                   mio::osecir::InfectionState::InfectedSymptoms,
                                   mio::osecir::InfectionState::Recovered};
    for (size_t county_idx_i = 0; county_idx_i < params_graph.nodes().size(); ++county_idx_i) {
        for (size_t county_idx_j = 0; county_idx_j < params_graph.nodes().size(); ++county_idx_j) {
            auto& populations = params_graph.nodes()[county_idx_i].property.populations;
            // mobility coefficients have the same number of components as the contact matrices.
            // so that the same NPIs/dampings can be used for both (e.g. more home office => fewer commuters)
            auto mobility_coeffs = mio::MigrationCoefficientGroup(contact_locations.size(), populations.numel());

            //commuters
            auto working_population = 0.0;
            auto min_commuter_age   = mio::AgeGroup(2);
            auto max_commuter_age   = mio::AgeGroup(4); //this group is partially retired, only partially commutes
            for (auto age = min_commuter_age; age <= max_commuter_age; ++age) {
                working_population += populations.get_group_total(age) * (age == max_commuter_age ? 0.33 : 1.0);
            }
            auto commuter_coeff_ij = mobility_data_commuter(county_idx_i, county_idx_j) /
                                     working_population; //data is absolute numbers, we need relative
            for (auto age = min_commuter_age; age <= max_commuter_age; ++age) {
                for (auto compartment : migrating_compartments) {
                    auto coeff_index = populations.get_flat_index({age, compartment});
                    mobility_coeffs[size_t(ContactLocation::Work)].get_baseline()[coeff_index] =
                        commuter_coeff_ij * (age == max_commuter_age ? 0.33 : 1.0);
                }
            }
            //others
            auto total_population = populations.get_total();
            auto twitter_coeff    = mobility_data_twitter(county_idx_i, county_idx_j) /
                                 total_population; //data is absolute numbers, we need relative
            for (auto age = mio::AgeGroup(0); age < populations.size<mio::AgeGroup>(); ++age) {
                for (auto compartment : migrating_compartments) {
                    auto coeff_idx = populations.get_flat_index({age, compartment});
                    mobility_coeffs[size_t(ContactLocation::Other)].get_baseline()[coeff_idx] = twitter_coeff;
                }
            }

            //only add edges with mobility above thresholds for performance
            //thresholds are chosen empirically so that more than 99% of mobility is covered, approx. 1/3 of the edges
            if (commuter_coeff_ij > 4e-5 || twitter_coeff > 1e-5) {
                params_graph.add_edge(county_idx_i, county_idx_j, std::move(mobility_coeffs));
            }
        }
    }

    return mio::success();
}

/**
 * Create the input graph for the parameter study.
 * Reads files from the data directory.
 * @param start_date start date of the simulation.
 * @param end_date end date of the simulation.
 * @param data_dir data directory.
 * @returns created graph or any io errors that happen during reading of the files.
 */
mio::IOResult<mio::Graph<mio::osecir::Model, mio::MigrationParameters>>
create_graph(mio::Date start_date, mio::Date end_date, const fs::path& data_dir)
{
    const auto start_day = mio::get_day_in_year(start_date);

    // global parameters
    const int num_age_groups = 6;
    mio::osecir::Parameters params(num_age_groups);
    params.get<mio::osecir::StartDay>() = start_day;
    BOOST_OUTCOME_TRY(set_covid_parameters(params));
    BOOST_OUTCOME_TRY(set_contact_matrices(data_dir, params));
    BOOST_OUTCOME_TRY(set_npis(start_date, end_date, params));

    // graph of counties with populations and local parameters
    // and mobility between counties
    mio::Graph<mio::osecir::Model, mio::MigrationParameters> params_graph;
    BOOST_OUTCOME_TRY(set_nodes(params, start_date, end_date, data_dir, params_graph));
    BOOST_OUTCOME_TRY(set_edges(data_dir, params_graph));

    return mio::success(params_graph);
}

/**
 * Different modes for running the parameter study.
 */
enum class RunMode
{
    Load,
    Save,
};

/**
 * Run the parameter study.
 * Load a previously stored graph or create a new one from data. 
 * The graph is the input for the parameter study.
 * A newly created graph is saved and can be reused.
 * @param mode Mode for running the parameter study.
 * @param data_dir data directory. Not used if mode is RunMode::Load.
 * @param save_dir directory where the graph is loaded from if mode is RunMOde::Load or save to if mode is RunMode::Save.
 * @param result_dir directory where all results of the parameter study will be stored.
 * @param save_single_runs [Default: true] Defines if single run results are written to the disk.
 * @returns any io error that occurs during reading or writing of files.
 */
mio::IOResult<void> run(RunMode mode, const fs::path& data_dir, const fs::path& save_dir, const fs::path& result_dir,
                        bool save_single_runs = true)
{
    const auto start_date   = mio::Date(2020, 12, 12);
    const auto num_days_sim = 20.0;
    const auto end_date     = mio::offset_date_by_days(start_date, int(std::ceil(num_days_sim)));
    const auto num_runs     = 1;

    //create or load graph
    mio::Graph<mio::osecir::Model, mio::MigrationParameters> params_graph;
    if (mode == RunMode::Save) {
        BOOST_OUTCOME_TRY(created, create_graph(start_date, end_date, data_dir));
        BOOST_OUTCOME_TRY(write_graph(created, save_dir.string()));
        params_graph = created;
    }
    else {
        BOOST_OUTCOME_TRY(loaded, mio::read_graph<mio::osecir::Model>(save_dir.string()));
        params_graph = loaded;
    }

    std::vector<int> county_ids(params_graph.nodes().size());
    std::transform(params_graph.nodes().begin(), params_graph.nodes().end(), county_ids.begin(), [](auto& n) {
        return n.id;
    });

    //run parameter study
    auto parameter_study =
        mio::ParameterStudy<mio::osecir::Simulation<>>{params_graph, 0.0, num_days_sim, 0.5, size_t(num_runs)};
    auto ensemble_results = std::vector<std::vector<mio::TimeSeries<double>>>{};
    ensemble_results.reserve(size_t(num_runs));
    auto ensemble_params = std::vector<std::vector<mio::osecir::Model>>{};
    ensemble_params.reserve(size_t(num_runs));
    auto save_result_result = mio::IOResult<void>(mio::success());
    auto run_idx            = size_t(0);
    parameter_study.run(
        [](auto&& graph) {
            return draw_sample(graph);
        },
        [&](auto results_graph) {
            ensemble_results.push_back(mio::interpolate_simulation_result(results_graph));

            ensemble_params.emplace_back();
            ensemble_params.back().reserve(results_graph.nodes().size());
            std::transform(results_graph.nodes().begin(), results_graph.nodes().end(),
                           std::back_inserter(ensemble_params.back()), [](auto&& node) {
                               return node.property.get_simulation().get_model();
                           });

            if (save_result_result && save_single_runs) {
                save_result_result = save_result_with_params(ensemble_results.back(), ensemble_params.back(),
                                                             county_ids, result_dir, run_idx);
            }
            ++run_idx;
        });
    BOOST_OUTCOME_TRY(save_result_result);
    BOOST_OUTCOME_TRY(save_results(ensemble_results, ensemble_params, county_ids, result_dir, save_single_runs));

    return mio::success();
}

int main(int argc, char** argv)
{
    //TODO: proper command line interface to set:
    //- number of runs
    //- start and end date (may be incompatible with runmode::load)
    //- seeds
    //- log level
    //- ...

    mio::set_log_level(mio::LogLevel::warn);

    RunMode mode;
    std::string save_dir;
    std::string data_dir;
    std::string result_dir;
    bool save_single_runs = true;
    if (argc == 5) {
        mode             = RunMode::Save;
        data_dir         = argv[1];
        save_dir         = argv[2];
        result_dir       = argv[3];
        if (atoi(argv[4]) == 0) {
            save_single_runs = false;
        }
        printf("\n Reading data from \"%s\", saving graph to \"%s\".\n", data_dir.c_str(), save_dir.c_str());
        printf("\n Exporting single run results and parameters: %d.\n", (int)save_single_runs);
    }
    else if (argc == 4) {
        mode       = RunMode::Save;
        data_dir   = argv[1];
        save_dir   = argv[2];
        result_dir = argv[3];
        printf("Reading data from \"%s\", saving graph to \"%s\".\n", data_dir.c_str(), save_dir.c_str());
        printf("Exporting single run results and parameters: %d.\n", (int)save_single_runs);
    }
    else if (argc == 3) {
        mode       = RunMode::Load;
        save_dir   = argv[1];
        result_dir = argv[2];
        data_dir   = "";
        printf("Loading graph from \"%s\".\n", save_dir.c_str());
        printf("Exporting single run results and parameters: %d.\n", (int)save_single_runs);
    }
    else {
        printf("Usage:\n");
        printf("2020_npis_wildtype <data_dir> <save_dir> <result_dir>\n");
        printf("\tMake graph with data from <data_dir> and save at <save_dir>, then run the simulation.\n");
        printf("\tStore the results in <result_dir>\n");
        printf("2020_npis_wildtype <load_dir> <result_dir>\n");
        printf("\tLoad graph from <load_dir>, then run the simulation.\n");
        return 0;
    }
    printf("Saving results to \"%s\".\n", result_dir.c_str());

    // mio::thread_local_rng().seed({...}); //set seeds, e.g., for debugging
    printf("Seeds: ");
    for (auto s : mio::thread_local_rng().get_seeds()) {
        printf("%u, ", s);
    }
    printf("\n");

    auto result = run(mode, data_dir, save_dir, result_dir, save_single_runs);
    if (!result) {
        printf("%s\n", result.error().formatted_message().c_str());
        return -1;
    }
    return 0;
}
