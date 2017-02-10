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

#include <classification/ModelMetadata.h>
#include <map>
#include <memory>

namespace dg {namespace deepcore { namespace classification {

enum Actions
{
    HELP,
    SHOW,
    PACK,
    UNPACK
};

struct GbdxmArgs
{
    Actions action;
    bool verbose;
    std::string gbdxFile;

    GbdxmArgs() : action(HELP), verbose(false) { }
};

struct GbdxmPackArgs : public GbdxmArgs
{
    std::unique_ptr<ModelMetadata> metadata;
    std::string labelsFile;
    std::map<std::string, std::string> modelFiles;
    bool encrypt = true;
};

struct GbdxmUnpackArgs : public GbdxmArgs
{
    std::string outputDir;
};

void printVerbose(const GbdxmArgs& args, const std::string& message);

void doAction(GbdxmArgs& args);

} } } // namespace dg {namespace deepcore { namespace classification {

#endif // DEEPCORE_GBDXM_H
