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

#include "gbdxm.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <classification/CaffeModelPackage.h>
#include <classification/GbdxModelReader.h>
#include <classification/GbdxModelWriter.h>
#include <utility/Error.h>
#include <utility/File.h>
#include <utility/Logging.h>

namespace dg {namespace gbdxm {

namespace fs = boost::filesystem;

using namespace dg::deepcore;

using std::cout;
using std::endl;
using std::ios;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

void showModel(const GbdxmArgs& args);
void packModel(GbdxmPackArgs& args);
void unpackModel(const GbdxmUnpackArgs& args);
void writeLabels(const string& fileName, const vector<string>& labels);

void doAction(GbdxmArgs& args)
{
    switch(args.action) {
        case Action::SHOW:
            showModel(args);
            break;

        case Action::PACK:
        {
            auto& packArgs = static_cast<GbdxmPackArgs&>(args);
            packModel(packArgs);
            break;
        }

        case Action::UNPACK:
        {
            auto& unpackArgs = static_cast<GbdxmUnpackArgs&>(args);
            unpackModel(unpackArgs);
            break;
        }

        default:
            // HELP would've been handled by command line arguments parser
            DG_ERROR_THROW("Invalid action");
    }
}

void showModel(const GbdxmArgs& args)
{
    DG_LOG(gbdxm, info) << "Showing metadata of " << args.gbdxFile;

    DG_CHECK(fs::exists(args.gbdxFile), "Input file does not exist at %s", args.gbdxFile.c_str());
    DG_CHECK(!fs::is_directory(args.gbdxFile), "Input file at %s is a directory", args.gbdxFile.c_str());

    DG_LOG(gbdxm, info) << "Opening " << args.gbdxFile;
    UnZipFile unzFile(args.gbdxFile);

    DG_LOG(gbdxm, info) << "Reading metadata.json";
    DG_CHECK(unzFile.fileName() == "metadata.json",
             "Expecting metadata.json, found %s instead", unzFile.fileName().c_str());

    auto metadata = unzFile.readFileToString();
    cout << metadata.c_str() << endl;

    DG_LOG(gbdxm, info) <<  "Done";
}

void packModel(GbdxmPackArgs& args)
{
    DG_LOG(gbdxm, info) << "Packing " << args.package->type() << " model to " << args.gbdxFile;

    // Make sure the output file can be created
    DG_CHECK(!fs::is_directory(args.gbdxFile), "Cannot write to output, %s is a directory.", args.gbdxFile);

    auto parentPath = fs::path(args.gbdxFile).parent_path();
    if(!parentPath.empty() && !fs::exists(parentPath)) {
        DG_LOG(gbdxm, info) << "Creating directory " << parentPath.string();
        fs::create_directories(parentPath);
    }

    auto& package = *args.package;
    auto& metadata = package.metadata();

    vector<Error> errors;

    // Check the input files
    if(args.labelsFile.empty() && metadata.labels().empty()) {
        errors.push_back(DG_ERROR_INIT("Labels not given in a file or JSON"));
    } else if(!args.labelsFile.empty() && (!fs::exists(args.labelsFile) || fs::is_directory(args.labelsFile))) {
        errors.push_back(DG_ERROR_INIT("Label file does not exist at '%s'", args.labelsFile.c_str()));
    }

    // Find out the total file size while we're checking the files
    int64_t totalFileSize = 0;
    for(const auto& mapItem : args.modelFiles) {
        if(!fs::exists(mapItem.second) || fs::is_directory(mapItem.second)) {
            errors.push_back(DG_ERROR_INIT("File does not exist for %s at '%s'", mapItem.first.c_str(), mapItem.second.c_str()));
            continue;
        }
        totalFileSize += fs::file_size(mapItem.second);
    }

    if(!errors.empty()) {
        for(auto it = errors.begin(); it != (errors.end() - 1); ++it) {
            DG_ERROR_LOG(gbdxm, *it);
        }

        throw errors.back();
    }

    metadata.setSize(totalFileSize);

    if(!args.labelsFile.empty()) {
        DG_LOG(gbdxm, info) << "Reading labels from " << args.labelsFile;
        metadata.setLabels(readLinesFromFile(args.labelsFile));
    }

    DG_LOG(gbdxm, info) << "Creating " << args.gbdxFile;
    classification::GbdxModelWriter writer(args.gbdxFile, package, args.encrypt);

    // Prepare the metadata map by stripping path, leaving just the file names.
    auto contentMap = args.modelFiles;
    for(auto& mapItem : contentMap) {
        mapItem.second = fs::path(mapItem.second).filename().string();
    }

    DG_LOG(gbdxm, info) << "Writing metadata";
    writer.writeMetadata(contentMap);

    // Add model files
    for(const auto& mapItem : args.modelFiles) {
        DG_LOG(gbdxm, info) << "Adding " << mapItem.first << " from " << mapItem.second;
        if(package.haveItem(mapItem.first)) {
            writer.addFile(mapItem.first, package.item(mapItem.first));
        } else {
            writer.addFile(mapItem.first, mapItem.second);
        }
    }

    DG_LOG(gbdxm, info) << "Closing " << args.gbdxFile;
    writer.close();

    DG_LOG(gbdxm, info) << "Done";
}

void unpackModel(const GbdxmUnpackArgs& args)
{
    DG_LOG(gbdxm, info) << "Unpacking " << args.gbdxFile << " to " << args.outputDir;

    // Make sure the input file exists
    DG_CHECK(fs::exists(args.gbdxFile), "Input file does not exist at %s",  args.gbdxFile.c_str());
    DG_CHECK(!fs::is_directory(args.gbdxFile), "Input file at %s is a directory", args.gbdxFile.c_str());

    // Make sure the output directory exists and create one if it doesn't
    if(!fs::is_directory(args.outputDir)) {
        DG_CHECK(!fs::exists(args.outputDir),
                 "Could not create output directory at %s, already a file.", args.outputDir.c_str());

        DG_LOG(gbdxm, info) << "Creating directory " << args.outputDir;
        fs::create_directories(args.outputDir);
    }

    // Read the model
    DG_LOG(gbdxm, info) << "Reading model from " << args.gbdxFile;
    classification::GbdxModelReader reader(args.gbdxFile);
    map<string, string> contentMap;
    auto package = reader.readModel(contentMap);

    auto fileName = fs::path(args.outputDir).append("labels.txt").string();
    writeLabels(fileName, package->metadata().labels());

    // Write out the model files
    for(const auto& mapItem : contentMap) {
        fileName = fs::path(args.outputDir).append(mapItem.second).string();

        DG_LOG(gbdxm, info) << "Writing " << mapItem.first << " to " << fileName;

        ofstream ofs(fileName, ios::binary);
        DG_CHECK(ofs.good(), "Error creating %s for writing model data: %s", fileName.c_str(), strerror(errno));

        const auto& modelData = package->item(mapItem.first);
        ofs.write(reinterpret_cast<const char*>(modelData.data()), modelData.size());
        DG_CHECK(ofs.good(), "Error writing model data to %s: %s", fileName.c_str(), strerror(errno));
    }

    DG_LOG(gbdxm, info) << "Done";
}

void writeLabels(const string& fileName, const vector<string>& labels)
{
    ofstream ofs(fileName);
    DG_CHECK(ofs.good(), "Error creating labels file at %s: %s", fileName.c_str(), strerror(errno));

    for(const auto& label : labels) {
        ofs << label << endl;
        DG_CHECK(ofs.good(), "Error writing labels to  %s: %s", fileName.c_str(), strerror(errno));
    }

    ofs.close();
    DG_CHECK(ofs.good(), "Error writing labels to  %s: %s", fileName.c_str(), strerror(errno));
}

} } // namespace dg { namespace gbdxm {
