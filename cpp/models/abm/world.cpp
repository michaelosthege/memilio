/* 
* Copyright (C) 2020-2021 German Aerospace Center (DLR-SC)
*        & Helmholtz Centre for Infection Research (HZI)
*
* Authors: Daniel Abele, Majid Abedi, Elisabeth Kluth
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
#include "abm/world.h"
#include "abm/person.h"
#include "abm/location.h"
#include "abm/migration_rules.h"
#include "memilio/utils/random_number_generator.h"
#include "memilio/utils/stl_util.h"

namespace mio
{
namespace abm
{

LocationId World::add_location(LocationType type, uint32_t num_cells)
{
    auto& locations = m_locations[(uint32_t)type];
    uint32_t index  = static_cast<uint32_t>(locations.size());
    locations.emplace_back(Location(type, index, num_cells));
    return {index, type};
}

Person& World::add_person(LocationId id, InfectionState infection_state, AgeGroup age)
{
    uint32_t person_id = static_cast<uint32_t>(m_persons.size());
    m_persons.push_back(std::make_unique<Person>(id, infection_state, age, m_infection_parameters,
                                                 VaccinationState::Unvaccinated, person_id));
    auto& person = *m_persons.back();
    get_location(person).add_person(person);
    return person;
}

void World::evolve(TimePoint t, TimeSpan dt)
{
    begin_step(t, dt);
    interaction(t, dt);
    m_testing_strategy.update_activity_status(t);
    migration(t, dt);
}

void World::interaction(TimePoint /*t*/, TimeSpan dt)
{
    for (auto&& person : m_persons) {
        auto& loc = get_location(*person);
        person->interact(dt, m_infection_parameters, loc);
    }
}

void World::set_infection_state(Person& person, InfectionState inf_state)
{
    auto& loc      = get_location(person);
    auto old_state = person.get_infection_state();
    person.set_infection_state(inf_state);
    loc.changed_state(person, old_state);
}

void World::migration(TimePoint t, TimeSpan dt)
{
    for (auto&& person : m_persons) {
        for (auto rule : m_migration_rules) {
            //check if transition rule can be applied
            const auto& locs = rule.second;
            bool nonempty    = !locs.empty();
            nonempty         = std::all_of(locs.begin(), locs.end(), [this](LocationType type) {
                return !m_locations[(uint32_t)type].empty();
            });
            if (nonempty) {
                auto target_type = rule.first(*person, t, dt, m_migration_parameters);
                Location* target = find_location(target_type, *person);
                if (m_testing_strategy.run_strategy(*person, *target)) {
                    if (target != &get_location(*person)) {
                        person->migrate_to(get_location(*person), *target);
                        break;
                    }
                }
            }
        }
    }
    // check if a person makes a trip
    size_t num_trips = m_trip_list.num_trips();
    if (num_trips != 0) {
        while (m_trip_list.get_current_index() < num_trips && m_trip_list.get_next_trip_time() < t + dt) {
            auto& trip   = m_trip_list.get_next_trip();
            auto& person = m_persons[trip.person_id];
            if (!person->is_in_quarantine() && person->get_location_id() == trip.migration_origin) {
                Location& target = get_individualized_location(trip.migration_destination);
                if (m_testing_strategy.run_strategy(*person, target)) {
                    person->migrate_to(get_location(*person), target);
                }
            }
            m_trip_list.increase_index();
        }
    }
}

void World::begin_step(TimePoint /*t*/, TimeSpan dt)
{
    for (auto&& locations : m_locations) {
        for (auto& location : locations) {
            location.begin_step(dt, m_infection_parameters);
        }
    }
}

auto World::get_locations() const -> Range<
    std::pair<std::vector<std::vector<Location>>::const_iterator, std::vector<std::vector<Location>>::const_iterator>>
{
    return std::make_pair(m_locations.begin(), m_locations.end());
}

auto World::get_persons() const -> Range<std::pair<ConstPersonIterator, ConstPersonIterator>>
{
    return std::make_pair(ConstPersonIterator(m_persons.begin()), ConstPersonIterator(m_persons.end()));
}

const Location& World::get_individualized_location(LocationId id) const
{
    return m_locations[(uint32_t)id.type][id.index];
}

Location& World::get_individualized_location(LocationId id)
{
    return m_locations[(uint32_t)id.type][id.index];
}

const Location& World::get_location(const Person& person) const
{
    return get_individualized_location(person.get_location_id());
}

Location* World::find_location(LocationType type, const Person& person)
{
    auto index = person.get_assigned_location_index(type);
    assert(index != INVALID_LOCATION_INDEX && "unexpected error.");
    return &get_individualized_location({index, type});
}

Location& World::get_location(const Person& person)
{
    return get_individualized_location(person.get_location_id());
}

int World::get_subpopulation_combined(InfectionState s, LocationType type) const
{
    auto& locs = m_locations[(uint32_t)type];
    return std::accumulate(locs.begin(), locs.end(), 0, [&](int running_sum, const Location& loc) {
        return running_sum + loc.get_subpopulation(s);
    });
}

MigrationParameters& World::get_migration_parameters()
{
    return m_migration_parameters;
}

const MigrationParameters& World::get_migration_parameters() const
{
    return m_migration_parameters;
}

GlobalInfectionParameters& World::get_global_infection_parameters()
{
    return m_infection_parameters;
}

const GlobalInfectionParameters& World::get_global_infection_parameters() const
{
    return m_infection_parameters;
}

TripList& World::get_trip_list()
{
    return m_trip_list;
}

const TripList& World::get_trip_list() const
{
    return m_trip_list;
}

void World::use_migration_rules(bool param)
{
    m_use_migration_rules = param;
    // Set up global migration rules for all agents
    // check if a person has to go to the hospital, ICU or home due to quarantine/recovery
    if (m_use_migration_rules) {
        m_migration_rules = {
            std::make_pair(&return_home_when_recovered,
                           std::vector<LocationType>{
                               LocationType::Home,
                               LocationType::Hospital}), //assumption: if there is an ICU, there is also an hospital
            std::make_pair(&go_to_hospital, std::vector<LocationType>{LocationType::Home, LocationType::Hospital}),
            std::make_pair(&go_to_icu, std::vector<LocationType>{LocationType::Hospital, LocationType::ICU}),
            std::make_pair(&go_to_school, std::vector<LocationType>{LocationType::School, LocationType::Home}),
            std::make_pair(&go_to_work, std::vector<LocationType>{LocationType::Home, LocationType::Work}),
            std::make_pair(&go_to_shop, std::vector<LocationType>{LocationType::Home, LocationType::BasicsShop}),
            std::make_pair(&go_to_event, std::vector<LocationType>{LocationType::Home, LocationType::SocialEvent}),
            std::make_pair(&go_to_quarantine, std::vector<LocationType>{LocationType::Home})};
    }
    else {
        m_migration_rules = {
            std::make_pair(&return_home_when_recovered,
                           std::vector<LocationType>{
                               LocationType::Home,
                               LocationType::Hospital}), //assumption: if there is an ICU, there is also an hospital
            std::make_pair(&go_to_hospital, std::vector<LocationType>{LocationType::Home, LocationType::Hospital}),
            std::make_pair(&go_to_icu, std::vector<LocationType>{LocationType::Hospital, LocationType::ICU}),
            std::make_pair(&go_to_quarantine, std::vector<LocationType>{LocationType::Home})};
    }
}

bool World::use_migration_rules() const
{
    return m_use_migration_rules;
}

TestingStrategy& World::get_testing_strategy()
{
    return m_testing_strategy;
}

const TestingStrategy& World::get_testing_strategy() const
{
    return m_testing_strategy;
}

} // namespace abm
} // namespace mio
