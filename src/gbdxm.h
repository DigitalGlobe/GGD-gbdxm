/********************************************************************************
* Copyright 2016 DigitalGlobe, Inc.
* Author: Aleksey Vitebskiy
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
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
