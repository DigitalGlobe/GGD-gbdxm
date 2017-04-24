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

#include <gbdxmVersion.h>

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <classification/CaffeModelPackage.h>
#include <classification/ModelMetadataJson.h>
#include <geometry/cv_program_options.hpp>
#include <json/json.h>

namespace po = boost::program_options;

using boost::algorithm::iequals;
using boost::algorithm::join;
using boost::algorithm::to_lower;
using boost::algorithm::trim;
using boost::conversion::try_lexical_convert;
using boost::filesystem::file_size;
using boost::filesystem::exists;
using boost::filesystem::is_directory;
using boost::posix_time::from_iso_string;
using boost::posix_time::from_time_t;
using boost::posix_time::to_iso_string;
using boost::posix_time::to_time_t;
using Json::Reader;
using Json::Value;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::find;
using std::ifstream;
using std::ios;
using std::move;
using std::remove_if;
using std::string;
using std::unique_ptr;
using std::vector;

namespace dg { namespace deepcore { namespace classification {

// Forward declarations
po::options_description buildVisibleOptions();
po::options_description buildHiddenOptions();
void addShowOptions(po::options_description& desc);
void addPackOptions(po::options_description& desc);
void addPackCaffeOptions(po::options_description& desc);
void addUnpackOptions(po::options_description& desc);

GbdxmArgs* readArgs(const po::variables_map& vm, const string& action);
GbdxmArgs* readShowArgs(const po::variables_map& vm);
GbdxmArgs* readPackArgs(const po::variables_map& vm);
vector<string> readJsonMetadata(const string& fileName, GbdxmPackArgs& args);
void readModelMetadata(GbdxmPackArgs& args, vector<string>& missingFields);
GbdxmArgs* readUnpackArgs(const po::variables_map& vm);
const vector<string>& modelItemNames(const string& type);
ColorMode parseColorMode(string arg);
void tryErase(vector<string>& names, const string& name);

} } } // namespace dg { namespace deepcore { namespace classification {

int main (int argc, const char* const* argv)
{
    using namespace dg::deepcore::classification;

    // Build argumens
    auto visible = buildVisibleOptions();
    auto hidden = buildHiddenOptions();

    po::options_description all;
    all.add(visible).add(hidden);

    // First argument is action, which we parse out ourselves
    if(argc < 2) {
        cout << visible << endl;

        cerr << "Must have at least 1 argument." << endl << endl;
        exit(1);
    }

    string action = argv[1];
    to_lower(action);

    // Add gbdxm-file as a positional argument for convenience
    po::positional_options_description p;
    p.add("gbdxm-file", -1);

    // Parse the arguments
    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc - 1, &argv[1])
            .extra_style_parser(&po::ignore_numbers)
            .options(all)
            .positional(p)
            .run(), vm);
        po::notify(vm);
    } catch(const exception& e) {
        cerr << e.what() << endl;
        exit(1);
    } catch(...) {
        cerr << "Unknown error parsing command line arguments." << endl;
        exit(1);
    }

    // Read arguments, if invalid action or "help", print the usage details
    unique_ptr<GbdxmArgs> args(readArgs(vm, action));
    if(!args) {
        if(action == "help") {
            cout << visible << endl;
            exit(0);
        } else {
            cout << visible << endl;
            cerr << "Invalid action. The correct actions are help, show, pack, and unpack." << endl << endl;
            exit(1);
        }
    }

    doAction(*args);
}

namespace dg { namespace deepcore { namespace classification {

po::options_description buildVisibleOptions()
{
    po::options_description desc(
        "GDBX Model Packaging Tool\n"
        "Version: " GBDXM_VERSION_STRING "\n\n"
        "Usage: gbdxm <action> [options] [gbdxm file]\n\n"
        "Actions:\n"
        "  help\t\t\t Show this help message.\n"
        "  show\t\t\t Show package metadata.\n"
        "  pack\t\t\t Pack a model into a GDBX package.\n"
        "  unpack\t\t Unpack a GBDX package and output the original model.\n\n"
        "General Options"
    );

    desc.add_options()
        ("verbose,v", "verbose output.")
        ("gbdxm-file,f", po::value<string>(), "input or output GBDX file.");

    addShowOptions(desc);
    addPackOptions(desc);
    addUnpackOptions(desc);

    return move(desc);
}

po::options_description buildHiddenOptions()
{
    po::options_description desc;
    desc.add_options()
        ("plaintext", "don't encrypt the model.");

    return move(desc);
}

void addShowOptions(po::options_description& desc)
{
    // No options here, left for future expansion
}

void addPackOptions(po::options_description& desc)
{
    po::options_description pack("Pack Options");
    pack.add_options()
        ("type,t", po::value<string>(), "Type of the input model. Currently supported types:\n \t - caffe")
        ("json,j", po::value<string>(), "Model metadata in JSON format. Command line parameters will override entries in this file if present.")
        ("name,n", po::value<string>(), "Model name.")
        ("version,V", po::value<string>(), "Model version.")
        ("description,d", po::value<string>(), "Model description.")
        ("labels,l", po::value<string>(), "Labels file name.")
        ("date-time", po::value<string>(), "Date/time the model was created (optional). Date/time is normally set "
            "automatically, specifying this argument is not recommended. Must be in ISO format, which is "
            "YYYYMMDDTHHMMSS,fffffffff where T is the date-time separator. i.e. 20020131T100001,123456789")
        ("model-size,w", po::cvSize_value(), "Classifier model size. Model parameters will "
            "override this if present. Must specify width and height. i.e. --model-size 128 128")
        ("bounding-box,b", po::cvRect2d_value(), "Training area bounding box. Must specify four "
            "coordinates: west longitude, south latitude, east longitude, and north latitude. i.e. "
            "--bounding-box -84.06163 37.22197 -84.038803 37.240162")
        ("image-type,i", po::value<string>(), "Image type. i.e. jpg")
        ("color-mode,c", po::value<string>(), "Color mode. Model parameters will override this if present. Must be one of the following: grayscale, rgb, multiband")
        ;

    addPackCaffeOptions(pack);

    desc.add(pack);
}

void addPackCaffeOptions(po::options_description& desc)
{
    po::options_description caffe("Pack Caffe Options");
    caffe.add_options()
        ("caffe-model", po::value<string>(), "Caffe model file name.")
        ("caffe-trained", po::value<string>(), "Caffe trained file name.")
        ("caffe-mean", po::value<string>(), "Caffe mean file name.");

    desc.add(caffe);
}

void addUnpackOptions(po::options_description& desc)
{
    po::options_description unpack("Unpack Options");
    unpack.add_options()
        ("output-dir,o", po::value<string>()->default_value("."), "Directory for the output model files. Default is current directory.");

    desc.add(unpack);
}

GbdxmArgs* readArgs(const boost::program_options::variables_map& vm, const string& action)
{
    unique_ptr<GbdxmArgs> args;

    try {
        if(action == "show") {
            args.reset(readShowArgs(vm));
        } else if(action == "pack") {
            args.reset(readPackArgs(vm));
        } else if(action == "unpack") {
            args.reset(readUnpackArgs(vm));
        }

        // If action is not in "show", "pack", or "unpack", just return nullptr
        if(!args) {
            return nullptr;
        }

        // --verbose
        if(vm.count("verbose")) {
            args->verbose = true;
        }

        // --gbdxm-file
        if(!vm.count("gbdxm-file")) {
            cerr << "No GBDX file specified." << endl;
            exit(1);
        }

        args->gbdxFile = vm["gbdxm-file"].as<string>();
    }
    catch(const exception& e) {
        cerr << "Error reading the arguments: " << e.what() << endl;
        exit(1);
    }

    return args.release();
}

GbdxmArgs* readShowArgs(const po::variables_map& vm)
{
    unique_ptr<GbdxmArgs> args(new GbdxmArgs);
    args->action = SHOW;

    return args.release();
}

GbdxmArgs* readPackArgs(const po::variables_map& vm)
{
    unique_ptr<GbdxmPackArgs> args(new GbdxmPackArgs);
    args->action = PACK;

    // --json
    vector<string> missingFields;
    if(vm.count("json")) {
        missingFields = readJsonMetadata(vm["json"].as<string>(), *args);
    }

    // --type
    if(!args->metadata) {
        if(!vm.count("type")) {
            cerr << "Missing model type." << endl;
            exit(1);
        }

        auto type = vm["type"].as<string>();
        to_lower(type);

        args->metadata.reset(ModelMetadata::create(type.c_str()));
        missingFields = ModelMetadataJson::fieldNames(type);
        tryErase(missingFields, "type");
        tryErase(missingFields, "version");
    }

    tryErase(missingFields, "size");

    // --version
    if(vm.count("version")) {
        args->metadata->setVersion(vm["version"].as<string>());
        tryErase(missingFields, "modelVersion");
    }

    // --name
    if(vm.count("name")) {
        args->metadata->setName(vm["name"].as<string>());
        tryErase(missingFields, "name");
    }

    // --description
    if(vm.count("description")) {
        args->metadata->setDescription(vm["description"].as<string>());
        tryErase(missingFields, "description");
    }

    // --labels
    if(vm.count("labels")) {
        args->labelsFile = vm["labels"].as<string>();
        if (!args->labelsFile.empty()) {
            tryErase(missingFields, "labels");
        }
    }

    // --date-time
    if(vm.count("date-time")) {
        time_t timeCreated;
        try {
            timeCreated = to_time_t(from_iso_string(vm["time-created"].as<string>()));
        } catch (...) {
            cerr << "Invalid date/time format in --date-time argument." << endl;
            exit(1);
        }
        args->metadata->setTimeCreated(timeCreated);
    } else if(find(missingFields.begin(), missingFields.end(), "timeCreated" ) != missingFields.end()){
        args->metadata->setTimeCreated(time(nullptr));
    }

    tryErase(missingFields, "timeCreated");

    // --model-size
    if(vm.count("model-size")) {
        args->metadata->setModelSize(vm["model-size"].as<cv::Size>());
        tryErase(missingFields, "modelSize");
    }

    // --bounding-box
    if(vm.count("bounding-box")) {
        args->metadata->setBoundingBox(vm["bounding-box"].as<cv::Rect2d>());
        tryErase(missingFields, "boundingBox");
    }

    // --image-type
    if(vm.count("image-type")) {
        args->metadata->setImageType(vm["image-type"].as<string>());
        tryErase(missingFields, "imageType");
    }

    // --color-mode
    if(vm.count("color-mode")) {
        args->metadata->setColorMode(parseColorMode(vm["color-mode"].as<string>()));
        tryErase(missingFields, "colorMode");
    }

    // "--<model>-<file>" arguments, i.e. "--caffe-model"
    auto itemNames = modelItemNames(args->metadata->type());
    vector<string> missingArgs;
    while(!itemNames.empty()) {
        auto itemName = itemNames.back();
        itemNames.pop_back();

        auto argName = string(args->metadata->type()) + "-" + itemName;
        if(vm.count(argName) && !vm[argName].as<string>().empty()) {
            args->modelFiles[itemName] = vm[argName].as<string>();
        } else if(args->modelFiles.find(itemName) == args->modelFiles.end()){
            missingArgs.push_back(argName);
        }
    }

    vector<string> errors;

    if(!missingArgs.empty()) {
        errors.push_back(string("Missing ") + args->metadata->type() + " model file arguments: --" + join(missingArgs, ", --"));
    } else {
        // Try to retrieve fields from the model
        readModelMetadata(*args, missingFields);
    }

    //create an error message for missingFields
    if(!missingFields.empty()) {
        auto cliMap = ModelMetadataJson::fieldToOption(args->metadata->type());
        vector<string> cliFields;

        for(std::string missingField : missingFields){
            auto it = cliMap.find(missingField);
            if(it != std::end(cliMap)) {
                cliFields.push_back(it->second);
            }
            else{
                cliFields.push_back(missingField);
            }
        }
        errors.push_back("Missing required metadata arguments: --" + join(cliFields, ", --"));
    }

    if(!errors.empty()) {
        for(const auto& error : errors) {
            cerr << error << endl;
        }
        exit(1);
    }

    if(vm.count("plaintext")) {
        args->encrypt = false;
    }

    return args.release();
}

vector<string> readJsonMetadata(const string& fileName, GbdxmPackArgs& args)
{
    if(!exists(fileName) || is_directory(fileName)) {
        cerr << "Json file does not exist at '" << fileName << "'" << endl;
        exit(1);
    }

    vector<string> missingFields;
    try {
        ifstream ifs(fileName);
        Reader reader;
        Value root;
        if(!reader.parse(ifs, root)) {
            cerr << "Error parsing metadata: " << reader.getFormattedErrorMessages() << endl;
            exit(1);
        }

        args.metadata = ModelMetadataJson::fromJsonPartial(root, missingFields);

        if(root.isMember("content")) {
            if(root["content"].type() != Json::objectValue) {
                cerr << "Invalid metadata content field: must be a JSON object." << endl;
            }

            for(const auto& name : root["content"].getMemberNames()) {
                args.modelFiles[name] = root["content"][name].asString();
            }
        }

    } catch(const std::exception& e) {
        cerr << "Error reading metadata from " << fileName << ": " << e.what() << endl;
        exit(1);
    }

    return missingFields;
}

void readModelMetadata(GbdxmPackArgs& args, vector<string>& missingFields)
{
    unique_ptr<ModelPackage> model(ModelPackage::create(*args.metadata));

    // go through model files that can contain metadata
    for(const auto& itemName : model->modelMetadataItemNames()) {
        const string& fileName = args.modelFiles[itemName];
        printVerbose(args, "Reading model metadata from " + fileName + "...");

        if(!exists(fileName) || is_directory(fileName)) {
            cerr << "File does not exist for " << itemName << " at '" << fileName << "'" << endl;
            exit(1);
        }

        auto length = file_size(fileName);

        ifstream ifs(fileName, ios::binary);
        if(ifs.bad()) {
            cerr << "Error opening " << itemName << " from '" << fileName << "'" << endl;
            exit(1);
        }

        vector<uint8_t> buf(static_cast<size_t>(length), 0);
        ifs.read(reinterpret_cast<char*>(buf.data()), length);
        if(ifs.bad()) {
            cerr << "Error reading " << itemName << " from '" << fileName << "'" << endl;
            exit(1);
        }

        // Read metadata from file contents
        auto itemsRead = model->readMetadataFromItem(itemName, buf);

        // Remove items retrieved from the missingFields list
        missingFields.erase(remove_if(missingFields.begin(), missingFields.end(), [&itemsRead](const string& item) {
            return find(itemsRead.begin(), itemsRead.end(), item) != itemsRead.end();
        }), missingFields.end());
    }

    // Reset the metadata to reflect changes
    args.metadata = model->metadata().clone();
}

GbdxmArgs* readUnpackArgs(const po::variables_map& vm)
{
    unique_ptr<GbdxmUnpackArgs> args(new GbdxmUnpackArgs);
    args->action = UNPACK;

    // --output-dir
    if(!vm.count("output-dir")) {
        cerr << "Missing output directory." << endl;
        exit(1);
    }

    args->outputDir = vm["output-dir"].as<string>();

    return args.release();
}

const vector<string>& modelItemNames(const string& type)
{
    if(type == "caffe") {
        return CaffeModelPackage::ITEM_NAMES;
    } else {
        cerr << "Unsupported model type: " << type.c_str() << "." << endl;
        exit(1);
    }
}

ColorMode parseColorMode(string arg)
{
    to_lower(arg);
    trim(arg);
    if(arg == "grayscale") {
        return ColorMode::GRAYSCALE;
    } else if(arg == "rgb") {
        return ColorMode ::RGB;
    } else if(arg == "multiband") {
        return ColorMode::MULTIBAND;
    }

    return ColorMode::UNKNOWN;
}

void tryErase(vector<string>& names, const string& name)
{
    auto it = find(names.begin(), names.end(), name);
    if(it != names.end()) {
        names.erase(it);
    }
}

} } } // namespace dg { namespace deepcore { namespace classification {