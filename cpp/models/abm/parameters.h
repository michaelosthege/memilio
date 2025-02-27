/* 
* Copyright (C) 2020-2021 German Aerospace Center (DLR-SC)
*
* Authors: Daniel Abele, Elisabeth Kluth
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
#ifndef EPI_ABM_PARAMETERS_H
#define EPI_ABM_PARAMETERS_H

#include "abm/age.h"
#include "abm/time.h"
#include "abm/state.h"
#include "memilio/utils/custom_index_array.h"
#include "memilio/math/eigen.h"
#include "memilio/utils/parameter_set.h"
#include "memilio/epidemiology/damping.h"
#include "memilio/epidemiology/contact_matrix.h"
#include <limits>

namespace mio
{
namespace abm
{

struct IncubationPeriod {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "IncubationPeriod";
    }
};

struct SusceptibleToExposedByCarrier {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "SusceptibleToExposedByCarrier";
    }
};

struct SusceptibleToExposedByInfected {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "SusceptibleToExposedByInfected";
    }
};

struct CarrierToInfected {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "CarrierToInfected";
    }
};

struct CarrierToRecovered {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "CarrierToRecovered";
    }
};

struct InfectedToRecovered {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "InfectedToRecovered";
    }
};

struct InfectedToSevere {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "InfectedToSevere";
    }
};

struct SevereToCritical {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "SevereToCritical";
    }
};

struct SevereToRecovered {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "SevereToRecovered";
    }
};

struct CriticalToRecovered {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "CriticalToRecovered";
    }
};

struct CriticalToDead {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 1.);
    }
    static std::string name()
    {
        return "CriticalToDead";
    }
};

struct RecoveredToSusceptible {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 0.);
    }
    static std::string name()
    {
        return "RecoveredToSusceptible";
    }
};

struct DetectInfection {
    using Type = CustomIndexArray<double, AgeGroup, VaccinationState>;
    static Type get_default()
    {
        return Type({AgeGroup::Count, VaccinationState::Count}, 0.5);
    }
    static std::string name()
    {
        return "DetectInfection";
    }
};

/**
 * parameters of the infection that are the same everywhere within the world.
 */
using GlobalInfectionParameters =
    ParameterSet<IncubationPeriod, SusceptibleToExposedByCarrier, SusceptibleToExposedByInfected, CarrierToInfected,
                 CarrierToRecovered, InfectedToRecovered, InfectedToSevere, SevereToCritical, SevereToRecovered,
                 CriticalToDead, CriticalToRecovered, RecoveredToSusceptible, DetectInfection>;

struct MaximumContacts {
    using Type = double;
    static constexpr Type get_default()
    {
        return std::numeric_limits<double>::max();
    }
    static std::string name()
    {
        return "MaximumContacts";
    }
};

/**
 * parameters of the infection that depend on the location.
 */
using LocalInfectionParameters = ParameterSet<MaximumContacts>;

struct TestParameters {
    double sensitivity;
    double specificity;
};

struct GenericTest {
    using Type = TestParameters;
    static constexpr Type get_default()
    {
        return Type{0.9, 0.99};
    }
    static std::string name()
    {
        return "GenericTest";
    }
};

struct AntigenTest : public GenericTest {
    using Type = TestParameters;
    static constexpr Type get_default()
    {
        return Type{0.8, 0.88};
    }
    static std::string name()
    {
        return "AntigenTest";
    }
};

struct PCRTest : public GenericTest {
    using Type = TestParameters;
    static constexpr Type get_default()
    {
        return Type{0.9, 0.99};
    }
    static std::string name()
    {
        return "PCRTest";
    }
};

/**
 * parameters that govern the migration between locations.
 */
struct LockdownDate {
    using Type = TimePoint;
    static auto get_default()
    {
        return TimePoint(std::numeric_limits<int>::max());
    }
    static std::string name()
    {
        return "LockdownDate";
    }
};
struct BasicShoppingRate {
    using Type = CustomIndexArray<double, AgeGroup>;
    static auto get_default()
    {
        return CustomIndexArray<double, AgeGroup>(AgeGroup::Count, 1.0);
    }
    static std::string name()
    {
        return "BasicShoppingRate";
    }
};
struct WorkRatio {
    using Type = DampingMatrixExpression<Dampings<Damping<ColumnVectorShape>>>;
    static auto get_default()
    {
        return Type(Eigen::VectorXd::Constant(1, 1.0));
    }
    static std::string name()
    {
        return "WorkRatio";
    }
};
struct SchoolRatio {
    using Type = DampingMatrixExpression<Dampings<Damping<ColumnVectorShape>>>;
    static auto get_default()
    {
        return Type(Eigen::VectorXd::Constant(1, 1.0));
    }
    static std::string name()
    {
        return "SchoolRatio";
    }
};
struct SocialEventRate {
    using Type = DampingMatrixExpression<Dampings<Damping<ColumnVectorShape>>>;
    static auto get_default()
    {
        return Type(Eigen::VectorXd::Constant((size_t)AgeGroup::Count, 1.0));
    }
    static std::string name()
    {
        return "SocialEventRate";
    }
};

struct GotoWorkTimeMinimum {
    using Type = CustomIndexArray<TimeSpan, AgeGroup>;
    static auto get_default()
    {
        return CustomIndexArray<TimeSpan, AgeGroup>(AgeGroup::Count, hours(6));
    }
    static std::string name()
    {
        return "GotoWorkTimeMinimum";
    }
};

struct GotoWorkTimeMaximum {
    using Type = CustomIndexArray<TimeSpan, AgeGroup>;
    static auto get_default()
    {
        return CustomIndexArray<TimeSpan, AgeGroup>(AgeGroup::Count, hours(9));
    }
    static std::string name()
    {
        return "GotoWorkTimeMaximum";
    }
};

struct GotoSchoolTimeMinimum {
    using Type = CustomIndexArray<TimeSpan, AgeGroup>;
    static auto get_default()
    {
        return CustomIndexArray<TimeSpan, AgeGroup>(AgeGroup::Count, hours(6));
    }
    static std::string name()
    {
        return "GotoSchoolTimeMinimum";
    }
};

struct GotoSchoolTimeMaximum {
    using Type = CustomIndexArray<TimeSpan, AgeGroup>;
    static auto get_default()
    {
        return CustomIndexArray<TimeSpan, AgeGroup>(AgeGroup::Count, hours(9));
    }
    static std::string name()
    {
        return "GotoSchoolTimeMaximum";
    }
};

/**
 * parameters that control the migration between locations.
 */
using MigrationParameters =
    ParameterSet<LockdownDate, SocialEventRate, BasicShoppingRate, WorkRatio, SchoolRatio, GotoWorkTimeMinimum,
                 GotoWorkTimeMaximum, GotoSchoolTimeMinimum, GotoSchoolTimeMaximum>;

} // namespace abm
} // namespace mio
#endif
