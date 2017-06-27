/********************************************************************************
* Copyright 2017 DigitalGlobe, Inc.
* Author: Aleksey Vitebskiy
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
********************************************************************************/

#ifndef DEEPCORE_GBDXM_H
#define DEEPCORE_GBDXM_H

#include <classification/ModelPackage.h>
#include <map>
#include <memory>

namespace dg { namespace gbdxm {

enum class Action
{
    HELP,
    SHOW,
    PACK,
    UNPACK
};

struct GbdxmArgs
{
    Action action = Action::HELP;
    std::string gbdxFile;
};

struct GbdxmPackArgs : public GbdxmArgs
{
    const deepcore::classification::ModelIdentifier* identifier = nullptr;
    std::unique_ptr<deepcore::classification::ModelPackage> package;
    std::string labelsFile;
    std::map<std::string, std::string> modelFiles;
    bool encrypt = true;
};

struct GbdxmUnpackArgs : public GbdxmArgs
{
    std::string outputDir;
};

void doAction(GbdxmArgs& args);

} } // namespace dg { namespace gbdxm {

#endif // DEEPCORE_GBDXM_H
