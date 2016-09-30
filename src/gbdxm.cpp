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

#include "gbdxm.h"

#include <classification/CaffeModelPackage.h>
#include <classification/GbdxModelReader.h>
#include <classification/GbdxModelWriter.h>
#include <utility/zip/UnZipFile.h>
#include <utility/Error.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>

namespace fs = boost::filesystem;

using boost::algorithm::trim;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ios;
using std::map;
using std::ofstream;
using std::string;
using std::unique_ptr;
using std::vector;

namespace dg {namespace deepcore { namespace classification {

void showModel(const GbdxmArgs& args);
void packModel(GbdxmPackArgs& args);
void unpackModel(const GbdxmUnpackArgs& args);
vector<string> readLabels(const string& fileName);
void writeLabels(const string& fileName, const vector<string>& labels);

void printVerbose(const GbdxmArgs& args, const std::string& message)
{
    if(args.verbose) {
        cout << message.c_str() << endl;
    }
}

void doAction(GbdxmArgs& args)
{
    switch(args.action) {
        case SHOW:
            showModel(args);
            break;

        case PACK:
        {
            auto& packArgs = static_cast<GbdxmPackArgs&>(args);
            packModel(packArgs);
            break;
        }

        case UNPACK:
        {
            auto& unpackArgs = static_cast<GbdxmUnpackArgs&>(args);
            unpackModel(unpackArgs);
            break;
        }

        case HELP:
        default:
            // HELP would've been handled by command line arguments parser
            cerr << "Invalid action." << endl;
            exit(1);
    }
}

void showModel(const GbdxmArgs& args)
{
    printVerbose(args, "Showing metadata of " + args.gbdxFile + "...");

    try {
        fs::path gbdxPath(args.gbdxFile);
        if(!fs::exists(gbdxPath)) {
            cerr << gbdxPath.c_str() << " does not exist." << endl;
            exit(1);
        }

        if(fs::is_directory(gbdxPath)) {
            cerr << gbdxPath.c_str() << " is  a directory." << endl;
            exit(1);
        }

        printVerbose(args, "Opening " + args.gbdxFile + "...");
        UnZipFile unzFile(args.gbdxFile);

        printVerbose(args, "Reading metadata.json ...");
        if(unzFile.fileName() != "metadata.json") {
            cerr << "Expecting metadata.json, found " << gbdxPath.c_str() << " instead." << endl;
            exit(1);
        }

        string metadata = unzFile.readFileToString();
        cout << metadata.c_str() << endl;
    } catch(const std::exception& e) {
        cerr << "Error showing the model: " << e.what() << endl;
        exit(1);
    } catch(...) {
        cerr << "Unkwnon error showing the model." << endl;
        exit(1);
    }

    printVerbose(args, "Done!");
}

void packModel(GbdxmPackArgs& args)
{
    printVerbose(args, "Packing " + string(args.metadata->type()) + " model to " + args.gbdxFile + "...");

    try {
        // Make sure the output file can be created
        if(fs::is_directory(args.gbdxFile)) {
            cerr << "Cannot write to output, " << args.gbdxFile << " is a directory.";
            exit(1);
        }

        auto parentPath = fs::path(args.gbdxFile).parent_path();
        if(!parentPath.empty() && !fs::exists(parentPath)) {
            printVerbose(args, "Creating directory " + parentPath.string() + "...");
            fs::create_directories(parentPath);
        }

        // Check the input files

        bool missingFiles = false;
        if(!args.labelsFile.empty()) {
            if(!fs::exists(args.labelsFile) || fs::is_directory(args.labelsFile)) {
                cerr << args.labelsFile << " does not exist." << endl;

            }
        }

        // Find out the total file size while we're checking the files
        int64_t totalFileSize = 0;
        for(const auto& mapItem : args.modelFiles) {
            if(!fs::exists(mapItem.second) || fs::is_directory(mapItem.second)) {
                cerr << mapItem.second << " does not exist." << endl;
                missingFiles = true;
                continue;
            }
            totalFileSize += fs::file_size(mapItem.second);
        }

        args.metadata->setSize(totalFileSize);

        if(missingFiles) {
            exit(1);
        }

        // Create the model
        unique_ptr<ModelPackage> model(ModelPackage::create(*args.metadata));

        if(!args.labelsFile.empty()) {
            printVerbose(args, "Reading labels from " + args.labelsFile + "...");
            model->metadata().setLabels(readLabels(args.labelsFile));
        }

        printVerbose(args, "Creating " + args.gbdxFile + "...");
        GbdxModelWriter writer(args.gbdxFile, *model, args.encrypt);

        // Prepare the metadata map by stripping path, leaving just the file names.
        auto contentMap = args.modelFiles;
        for(auto& mapItem : contentMap) {
            mapItem.second = fs::path(mapItem.second).filename().string();
        }

        printVerbose(args, "Writing metadata...");
        writer.writeMetadata(contentMap);

        // Add model files
        for(const auto& mapItem : args.modelFiles) {
            printVerbose(args, "Adding " + mapItem.first + " from " + mapItem.second + "...");
            writer.addFile(mapItem.first, mapItem.second);
        }

        printVerbose(args, "Closing " + args.gbdxFile + "...");
        writer.close();
    } catch(const std::exception& e) {
        cerr << "Error packing the model: " <<e.what() << endl;
        exit(1);
    } catch(...) {
        cerr << "Unknown error packing the model." << endl;
        exit(1);
    }

    printVerbose(args, "Done!");
}

void unpackModel(const GbdxmUnpackArgs& args)
{
    printVerbose(args, "Unpacking " +  args.gbdxFile + " to " + args.outputDir + "...");

    try {
        // Make sure the input file exists
        if(!fs::exists(fs::absolute(args.gbdxFile).parent_path())) {
            cerr << args.gbdxFile << " does not exist." << endl;
            exit(1);
        }

        if(fs::is_directory(args.gbdxFile)) {
            cerr << args.gbdxFile << " is a directory." << endl;
            exit(1);
        }

        // Make sure the output directory exists and create one if it doesn't
        if(!fs::is_directory(args.outputDir)) {
            if(fs::exists(args.outputDir)) {
                cerr << args.outputDir << " exists, but it's not a directory." << endl;
                exit(1);
            }

            printVerbose(args, "Creating directory " + args.outputDir + "...");
            fs::create_directories(args.outputDir);
        }

        // Read the model
        printVerbose(args, "Reading model from " + args.gbdxFile + "...");
        GbdxModelReader reader(args.gbdxFile);
        map<string, string> contentMap;
        unique_ptr<ModelPackage> model(reader.readModel(contentMap));

        auto fileName = fs::path(args.outputDir).append("labels.txt").string();
        writeLabels(fileName, model->metadata().labels());

        // Write out the model files
        for(const auto& mapItem : contentMap) {
            fileName = fs::path(args.outputDir).append(mapItem.second).string();

            printVerbose(args, "Writing " + mapItem.first + " to " + fileName + "...");

            ofstream ofs(fileName, ios::binary);
            if(ofs.bad()) {
                cerr << "Error creating " << fileName.c_str() << "." << endl;
                exit(1);
            }

            const auto& modelData = model->item(mapItem.first);
            ofs.write(reinterpret_cast<const char*>(modelData.data()), modelData.size());
            if(ofs.bad()) {
                cerr << "Error writing model data to " << fileName << "." << endl;
                exit(1);
            }
        }
    } catch(const std::exception& e) {
        cerr << "Error unpacking the model: " << e.what() << endl;
        exit(1);
    } catch(...) {
        cerr << "Unknown error unpacking the model." << endl;
        exit(1);
    }

    printVerbose(args, "Done!");
}

vector<string> readLabels(const string& fileName)
{
    ifstream ifs(fileName);
    if(ifs.bad()) {
        cerr << "Error opening " << fileName << " for reading." << endl;
        exit(1);
    }

    vector<string> labels;
    string line;
    while(getline(ifs, line)) {
        trim(line);
        if(!line.empty()) {
            labels.push_back(move(line));
        }

        if(ifs.bad()) {
            cerr << "Error reading from " << fileName << "." << endl;
            exit(1);
        }
    }

    return move(labels);
}

void writeLabels(const string& fileName, const vector<string>& labels)
{
    ofstream ofs(fileName);
    if(ofs.bad()) {
        cerr << "Error creating " << fileName << "." << endl;
        exit(1);
    }

    for(const auto& label : labels) {
        ofs << label << endl;
        if(ofs.bad()) {
            cerr << "Error writing to " << fileName << "." << endl;
            exit(1);
        }
    }

    ofs.close();
    if(ofs.bad()) {
        cerr << "Error writing to " << fileName << "." << endl;
        exit(1);
    }
}

} } } // namespace dg {namespace deepcore { namespace classification {