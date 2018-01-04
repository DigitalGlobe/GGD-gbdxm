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

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/make_unique.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptor/map.hpp>
#include <geometry/cv_program_options.hpp>
#include <json/json.h>
#include <DeepCoreVersion.h>
#include <classification/Classification.h>
#include <classification/GbdxmCommon.h>
#include <classification/ModelMetadataJson.h>
#include <sstream>
#include <utility/File.h>
#include <utility/Error.h>
#include <utility/Logging.h>

namespace po = boost::program_options;

using namespace dg::deepcore;

using boost::adaptors::keys;
using boost::algorithm::join;
using boost::algorithm::to_lower;
using boost::algorithm::to_lower_copy;
using boost::filesystem::exists;
using boost::filesystem::is_directory;
using boost::make_unique;
using boost::posix_time::from_iso_string;
using boost::posix_time::to_time_t;
using std::cout;
using std::endl;
using std::find;
using std::find_if;
using std::ifstream;
using std::map;
using std::move;
using std::remove_if;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;

namespace dg { namespace gbdxm {

// Forward declarations

po::detail::cmdline::style_parser buildExtraStyleParser();
po::options_description buildVisibleOptions();
po::options_description buildHiddenOptions();
void addShowOptions(po::options_description& desc);
void addPackOptions(po::options_description& desc);
boost::shared_ptr<po::option_description> createFrameworkOption(classification::ItemDescription item, const char* type, const char* name, const char* ending);
void addPackFrameworkOptions(po::options_description& desc);
void addUnpackOptions(po::options_description& desc);

void setupLogging(const po::variables_map& vm);
unique_ptr<GbdxmArgs> readArgs(const po::variables_map& vm, const string& action);
unique_ptr<GbdxmArgs> readShowArgs(const po::variables_map& vm);
unique_ptr<GbdxmArgs> readPackArgs(const po::variables_map& vm);
vector<string> readJsonMetadata(const string& fileName, GbdxmPackArgs& args);
void readModelMetadata(GbdxmPackArgs& args, vector<string>& missingFields);
unique_ptr<GbdxmArgs> readUnpackArgs(const po::variables_map& vm);
void tryErase(vector<string>& names, const string& name);

} } // namespace dg { namespace gbdxm {

int main (int argc, const char* const* argv)
{
    using namespace dg::gbdxm;

    try {
        // Setup initial logging environment
        log::init();
        log::addCerrSink(level_t::warning, level_t::fatal, log::dg_log_format::dg_short_log);

        classification::init();

        // Build arguments
        auto visible = buildVisibleOptions();
        auto hidden = buildHiddenOptions();

        po::options_description all;
        all.add(visible).add(hidden);

        // First argument is action, which we parse out ourselves
        if(argc < 2) {
            cout << visible << endl;
            DG_ERROR_THROW("Must have at least 1 argument.");
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
                          .extra_style_parser(buildExtraStyleParser())
                          .options(all)
                          .positional(p)
                          .run(), vm);
            po::notify(vm);
        } catch(...) {
            cout << visible << endl;
            DG_ERROR_RETHROW("");
        }

        setupLogging(vm);

        // Read arguments, if invalid action or "help", print the usage details
        auto args = readArgs(vm, action);
        if(!args) {
            cout << visible << endl;
            DG_CHECK(action == "help", "Invalid action. The correct actions are help, show, pack, and unpack");

            exit(0);
        }

        doAction(*args);
    } catch (...) {
        DG_ERROR_LOG(gbdxm, DG_ERROR_FROM_CURRENT(""));
        return 1;
    }

    return 0;
}

namespace dg { namespace gbdxm {

po::detail::cmdline::style_parser buildExtraStyleParser()
{
    vector<po::detail::cmdline::style_parser> parsers;

    parsers.emplace_back(&po::ignore_numbers);

    for(const auto& type : classification::ModelIdentifier::types()) {
        parsers.emplace_back(po::prefix_argument(type));
    }

    return po::combine_style_parsers(parsers);
}

po::options_description buildVisibleOptions()
{
    stringstream ss;
    ss <<
        "GDBX Model Packaging Tool\n"
        "Version: " GBDXM_VERSION_STRING "\n"
        "Built on DeepCore version: " DEEPCORE_VERSION_STRING "\n"
        "GBDXM Metadata Version: " << classification::gbdxm::METADATA_VERSION << "\n\n"
        "Usage: gbdxm <action> [options] [gbdxm file]\n\n"
        "Actions:\n"
        "  help  \t\t Show this help message.\n"
        "  show  \t\t Show package metadata.\n"
        "  pack  \t\t Pack a model into a GBDX package.\n"
        "  unpack\t\t Unpack a GBDX package and output the original model.\n\n"
        "General Options";

    po::options_description desc(
        ss.str()
    );

    desc.add_options()
        ("verbose,v", "Verbose output.")
        ("gbdxm-file,f", po::value<string>()->value_name("PATH"), "Input or output GBDXM file.");

    addShowOptions(desc);
    addPackOptions(desc);
    addUnpackOptions(desc);

    return move(desc);
}

po::options_description buildHiddenOptions()
{
    po::options_description desc;
    desc.add_options()
        ("plaintext", "don't encrypt the model.")
        ("image-type,i", po::value<string>()->value_name("TYPE"), "Image type. e.g. jpg (deprecated).");

    return move(desc);
}

void addShowOptions(po::options_description& desc)
{
    // No options here, left for future expansion
}

void addPackOptions(po::options_description& desc)
{
    po::options_description pack("Pack Options");
    string supportedTypes = "Type of the input model. Currently supported types:";
    for(const auto& type : classification::ModelIdentifier::types()) {
        supportedTypes += "\n \t - " + type;
    }

    pack.add_options()
        ("type,t", po::value<string>()->value_name("TYPE"), supportedTypes.c_str())
        ("json,j", po::value<string>()->value_name("PATH"),
            "Model metadata in JSON format. Command line parameters will override "
            "entries in this file if present.")
        ("name,n", po::value<string>()->value_name("NAME"), "Model name.")
        ("category,C", po::value<string>()->value_name("CATEGORY"), "Model category.")
        ("version,V", po::value<string>()->value_name("VERSION"), "Model version.")
        ("description,d", po::value<string>()->value_name("DESCRIPTION"), "Model description.")
        ("labels,l", po::value<string>()->value_name("PATH"), "Labels file name.")
        ("label-names", po::value<vector<string>>()->multitoken()->value_name("LABEL1 [LABEL2 ...]"),
            "A list of label names.")
        ("date-time", po::value<string>()->value_name("DATE_TIME"),
            "Date/time the model was created (optional). Default is the current date and time. Must be in the following "
            "ISO format: YYYYMMDDTHHMMSS[.ffffff], where 'T' is the literal date-time separator. e.g. 20020131T100001.123456")
        ("model-size,w", po::cvSize_value()->value_name("WIDTH [HEIGHT]"), "Classifier model size. Model parameters will "
            "override this if present.")
        ("bounding-box,b", po::cvRect2d_value()->value_name("W S E N"),
            "Training area bounding box (optional). Must specify four "
            "coordinates: west longitude, south latitude, east longitude, and north latitude. e.g. "
            "--bounding-box -180 -90 180 90")
        ("color-mode,c", po::value<string>()->value_name("MODE"),
            "Color mode. Model parameters will override this if present. Must be one of the following: grayscale, rgb, multiband")
        ("resolution,r", po::cvSize2d_value()->value_name("WIDTH [HEIGHT]"),
            "Model pixel resolution (optional).")
        ;

    addPackFrameworkOptions(pack);

    desc.add(pack);
}



void addPackFrameworkOptions(po::options_description& desc)
{
    for(const auto& type : classification::ModelIdentifier::types()) {
        const auto& identifier = *classification::ModelIdentifier::find(type);

        string title("Pack ");
        title += identifier.prettyType();
        title += " Options";

        po::options_description framework(title.c_str());

        auto multiDesc = string("Set multiple ") + identifier.prettyType() + " options.";

        framework.add(boost::make_shared<po::option_description>(
            type.c_str(),
            po::value<vector<string>>()
                ->value_name("NAME VALUE [NAME VALUE ...]")
                ->multitoken(),
            multiDesc.c_str()));

        // Add descriptions for model items
        for(const auto& item : identifier.itemDescriptions()) {
            framework.add(createFrameworkOption(item, identifier.type(), "PATH", " file name"));
        }

        // Add descriptions for model options
        for(const auto& item : identifier.optionDescriptions()) {
            framework.add(createFrameworkOption(item, identifier.type(), "VALUE", ""));
        }

        desc.add(framework);
    }
}

boost::shared_ptr<po::option_description> createFrameworkOption(classification::ItemDescription item, const char* type, const char* name, const char* ending)
{
    string option(type);
    option += "-";
    option += item.name;

    auto description = item.description + ending;
    if(item.optional) {
        description += " (optional)";
    }

    description += ".";

    return boost::make_shared<po::option_description>(option.c_str(), po::value<string>()->value_name(name), description.c_str());
}

void addUnpackOptions(po::options_description& desc)
{
    po::options_description unpack("Unpack Options");
    unpack.add_options()
        ("output-dir,o", po::value<string>()->value_name("PATH")->default_value("."),
            "Directory for the output model files. Default is current directory.");

    desc.add(unpack);
}

void setupLogging(const po::variables_map& vm)
{
    // --verbose
    if(vm.count("verbose")) {
        log::addCoutSink(level_t::info, level_t::info, log::dg_log_format::dg_short_log);
    }
}

unique_ptr<GbdxmArgs> readArgs(const boost::program_options::variables_map& vm, const string& action)
{
    unique_ptr<GbdxmArgs> args;

    if(action == "show") {
        args = readShowArgs(vm);
    } else if(action == "pack") {
        args = readPackArgs(vm);
    } else if(action == "unpack") {
        args = readUnpackArgs(vm);
    }

    // If action is not in "show", "pack", or "unpack", just return nullptr
    if(!args) {
        return nullptr;
    }

    // --gbdxm-file
    DG_CHECK(vm.count("gbdxm-file") > 0, "No GBDXM file specified.");
    args->gbdxFile = vm["gbdxm-file"].as<string>();

    return args;
}

unique_ptr<GbdxmArgs> readShowArgs(const po::variables_map& vm)
{
    unique_ptr<GbdxmArgs> args(new GbdxmArgs);
    args->action = Action::SHOW;

    return args;
}

void readFrameworkPackItems(map<string, string>& items,
                            map<string, string>& options,
                            vector<string>& missingArgs,
                            const classification::ItemDescriptions& descriptions,
                            const string& prefix)
{
    for(const auto& item : descriptions) {
        auto it = options.find(item.name);
        if(it != options.end()) {
            items[it->first] = it->second;
            options.erase(it);
        } else if(!item.optional && items.find(item.name) == items.end()) {
            missingArgs.push_back(prefix + item.name);
        }
    }
}

vector<string> readFrameworkPackArgs(const po::variables_map& vm, GbdxmPackArgs& args)
{
    auto type = args.package->type();
    map<string, string> cliOptions;
    if(vm.count(type) > 0) {
        const auto& argList = vm[args.package->type()].as<vector<string>>();
        DG_CHECK(argList.size() % 2 == 0, "--%s must have an even number of arguments", type);

        for(auto it = argList.begin(); it != argList.end(); ++it) {
            const auto& key = *it++;
            const auto& value = *it;
            cliOptions[key] = value;
        }
    }

    auto prefix = string(type) + "-";
    vector<string> missingArgs;

    readFrameworkPackItems(args.modelFiles, cliOptions, missingArgs, args.identifier->itemDescriptions(), prefix);

    map<string, string> options;
    readFrameworkPackItems(options, cliOptions, missingArgs, args.identifier->optionDescriptions(), prefix);
    args.package->metadata().setOptions(options);

    DG_CHECK(cliOptions.empty(), "Invalid %s options: %s",
            args.identifier->type(), join(keys(cliOptions), ", ").c_str());

    return missingArgs;
}

unique_ptr<GbdxmArgs> readPackArgs(const po::variables_map& vm)
{
    auto args = make_unique<GbdxmPackArgs>();
    args->action = Action::PACK;

    // --json
    vector<string> missingFields;
    if(vm.count("json")) {
        missingFields = readJsonMetadata(vm["json"].as<string>(), *args);
    }

    // --type
    if(!args->package) {
        DG_CHECK(vm.count("type") == 1, "Missing model type");

        auto type = vm["type"].as<string>();
        to_lower(type);

        args->package = classification::ModelPackage::create(type.c_str());
        missingFields = classification::ModelMetadataJson::fieldNames(type);
        tryErase(missingFields, "type");
        tryErase(missingFields, "version");
    }

    args->identifier = &args->package->identifier();

    tryErase(missingFields, "size");

    auto& metadata = args->package->metadata();

    // --version
    if(vm.count("version")) {
        metadata.setVersion(vm["version"].as<string>());
        tryErase(missingFields, "modelVersion");
    }

    // --name
    if(vm.count("name")) {
        metadata.setName(vm["name"].as<string>());
        tryErase(missingFields, "name");
    }

    // --category
    if(vm.count("category")) {
        metadata.setCategory(to_lower_copy(vm["category"].as<string>()));
        tryErase(missingFields, "category");
    }

    // --description
    if(vm.count("description")) {
        metadata.setDescription(vm["description"].as<string>());
        tryErase(missingFields, "description");
    }

    // --label-names
    if(vm.count("label-names")) {
        metadata.setLabels(vm["label-names"].as<vector<string>>());
        if(!metadata.labels().empty()) {
            tryErase(missingFields, "labels");
        }
    }

    // --labels
    if(vm.count("labels")) {
        DG_CHECK(vm.count("label-names") == 0, "Please specify either --labels or --label-names, not both");
        args->labelsFile = vm["labels"].as<string>();
        if (!args->labelsFile.empty()) {
            tryErase(missingFields, "labels");
        }
    }

    // --date-time
    if(vm.count("date-time")) {
        time_t timeCreated;
        try {
            timeCreated = to_time_t(from_iso_string(vm["date-time"].as<string>()));
        } catch (...) {
            DG_ERROR_THROW("Invalid date/time format in --date-time argument");
        }
        metadata.setTimeCreated(timeCreated);
    } else if(find(missingFields.begin(), missingFields.end(), "timeCreated" ) != missingFields.end()){
        metadata.setTimeCreated(time(nullptr));
    }

    tryErase(missingFields, "timeCreated");

    // --model-size
    if(vm.count("model-size")) {
        metadata.setModelSize(vm["model-size"].as<cv::Size>());
        tryErase(missingFields, "modelSize");
    }

    // --bounding-box
    if(vm.count("bounding-box")) {
        metadata.setBoundingBox(vm["bounding-box"].as<cv::Rect2d>());
        tryErase(missingFields, "boundingBox");
    }

    // --image-type
    if(vm.count("image-type")) {
        DG_LOG(gbdxm, warning) << "--image-type argument is deprecated and ignored";
    }

    // --color-mode
    if(vm.count("color-mode")) {
        metadata.setColorMode(classification::colorModeFromString(vm["color-mode"].as<string>()));
        DG_CHECK(metadata.colorMode() != classification::ColorMode::UNKNOWN,
                "Invalid --color-mode argument");

        tryErase(missingFields, "colorMode");
    }

    // --resolution
    if(vm.count("resolution")) {
        metadata.setResolution(vm["resolution"].as<cv::Size2d>());
    }

    // "--<type>-<option>" arguments, e.g. "--caffe-model"
    auto missingArgs = readFrameworkPackArgs(vm, *args);
    vector<string> errors;

    if(!missingArgs.empty()) {
        errors.push_back(string("Missing ") + metadata.type() + " model arguments: --" + join(missingArgs, ", --"));
    } else {
        // Try to retrieve fields from the model
        readModelMetadata(*args, missingFields);
    }

    //create an error message for missingFields
    if(!missingFields.empty()) {
        auto cliMap = classification::ModelMetadataJson::fieldToOption(metadata.type());
        vector<string> cliFields;

        for(const auto& missingField : missingFields){
            auto it = cliMap.find(missingField);
            if(it != end(cliMap)) {
                cliFields.push_back(it->second);
            }
            else{
                cliFields.push_back(missingField);
            }
        }
        errors.push_back("Missing required metadata arguments: --" + join(cliFields, ", --"));
    }

    auto categories = args->identifier->detectCategory(*args->package);
    if(categories.empty()) {
        if(metadata.category().empty()) {
            errors.emplace_back("Invalid or unsupported model: category could not be detected");
        } else {
            errors.emplace_back("Invalid or unsupported model");
        }
    } else if(!metadata.category().empty()) {
        if(find(categories.begin(), categories.end(), metadata.category()) == categories.end()) {
            errors.push_back(
                "Model category '" + metadata.category()  + "' is invalid,"
                " possible categories for this model are: " + join(categories, ", "));
        }
    } else if(categories.size() > 1) {
        errors.push_back("Please specify a model category, possible categories "
                         "for this model are: " + join(categories, ", "));
    } else {
        metadata.setCategory(categories[0]);
    }

    DG_CHECK(errors.empty(), "%s", join(errors, "\n").c_str());

    if(vm.count("plaintext")) {
        args->encrypt = false;
    }

    return std::move(args);
}

vector<string> readJsonMetadata(const string& fileName, GbdxmPackArgs& args)
{
    DG_CHECK(exists(fileName), "%s does not exist", fileName.c_str());
    DG_CHECK(!is_directory(fileName), "%s is a directory", fileName.c_str());

    vector<string> missingFields;
    ifstream ifs(fileName);
    DG_CHECK(ifs.good(), "Error opening %s: %s", fileName.c_str(), strerror(errno));

    Json::Reader reader;
    Json::Value root;
    DG_CHECK(reader.parse(ifs, root), "Error parsing metadata: %s",
             reader.getFormattedErrorMessages().c_str());

    auto metadata = classification::ModelMetadataJson::fromJsonPartial(root, missingFields);
    args.package = classification::ModelPackage::create(move(metadata));

    if(root.isMember("content")) {
        DG_CHECK(root["content"].type() == Json::objectValue,
                 "Invalid metadata \"content\" field: must be a JSON object.");

        for(const auto& name : root["content"].getMemberNames()) {
            args.modelFiles[name] = root["content"][name].asString();
        }
    }

    return missingFields;
}

void readModelMetadata(GbdxmPackArgs& args, vector<string>& missingFields)
{
    // Load the files with metadata into the ModelPackage
    for(const auto& itemName : args.identifier->metadataItems()) {
        const string& fileName = args.modelFiles[itemName];

        const auto& descriptions = args.identifier->itemDescriptions();
        auto it = find_if(descriptions.begin(), descriptions.end(), [&itemName](const classification::ItemDescription& desc) {
            return desc.name == itemName;
        });

        // Sanity check, should never happen unless the ModelPackage
        // and ModelIdentifier are set up wrong.
        DG_CHECK(it != descriptions.end(), "'%s' is not registered as valid model package item", itemName.c_str());

        if(fileName.empty()) {
            DG_CHECK(it->optional, "--%s-%s argument is missing", args.identifier->type(), itemName.c_str());
            continue;
        }

        if(!exists(fileName)) {
            if(it->optional) {
                DG_LOG(gbdxm, warning) << "Invalid --" << args.identifier->type() << "-" << itemName
                                       << " argument: "<< fileName << " does not exist";
                continue;
            } else {
                // Again, this shouldn't happen, but we'll handle it here anyway.
                DG_ERROR_THROW("Invalid --%s-%s argument: %s does not exist",
                               args.identifier->type(), itemName.c_str(), fileName.c_str());
            }
        }

        DG_LOG(gbdxm, info) << "Reading model metadata from " << fileName;

        args.package->setItem(itemName, readBinaryFile(fileName));
    }

    // Read the metadata
    auto itemsRead = args.identifier->readMetadata(*args.package);

    // Remove items retrieved from the missingFields list
    missingFields.erase(remove_if(missingFields.begin(), missingFields.end(), [&itemsRead](const string& item) {
        return find(itemsRead.begin(), itemsRead.end(), item) != itemsRead.end();
    }), missingFields.end());
}

unique_ptr<GbdxmArgs> readUnpackArgs(const po::variables_map& vm)
{
    unique_ptr<GbdxmUnpackArgs> args(new GbdxmUnpackArgs);
    args->action = Action::UNPACK;

    // --output-dir
    args->outputDir = vm["output-dir"].as<string>();

    return std::move(args);
}

void tryErase(vector<string>& names, const string& name)
{
    auto it = find(names.begin(), names.end(), name);
    if(it != names.end()) {
        names.erase(it);
    }
}

} } // namespace dg { namespace gbdxm {
