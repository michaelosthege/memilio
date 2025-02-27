/* 
* Copyright (C) 2020-2021 German Aerospace Center (DLR-SC)
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
#ifndef EPI_ABM_AGE_H
#define EPI_ABM_AGE_H

namespace mio
{
namespace abm
{

/**
 * age groups like RKI.
 * EXPERIMENTAL; will be merged with new model framework soon.
 */
enum class AgeGroup
{
    Age0to4 = 0,
    Age5to14,
    Age15to34,
    Age35to59,
    Age60to79,
    Age80plus,

    Count
};

} // namespace abm
} // namespace mio

#endif //EPI_ABM_AGE_H
