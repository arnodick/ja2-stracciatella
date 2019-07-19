#include <string>
#include "FL/Fl_Native_File_Chooser.H"
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_ask.H>
#include "logo32.png.h"
#include "slog/slog.h"
#include "RustInterface.h"

#include "Launcher.h"

#define LAUNCHER_TOPIC DEBUG_TAG_LAUNCHER

#define RESOLUTION_SEPARATOR "x"


const char* defaultResolution = "640x480";

const std::vector<GameVersion> predefinedVersions = {
	GameVersion::DUTCH,
	GameVersion::ENGLISH,
	GameVersion::FRENCH,
	GameVersion::GERMAN,
	GameVersion::ITALIAN,
	GameVersion::POLISH,
	GameVersion::RUSSIAN,
	GameVersion::RUSSIAN_GOLD
};
const std::vector< std::pair<int, int> > predefinedResolutions = {
	std::make_pair(640,  480),
	std::make_pair(800,  600),
	std::make_pair(1024, 768),
	std::make_pair(1280, 720),
	std::make_pair(1600, 900),
	std::make_pair(1920, 1080)
};
const std::vector<VideoScaleQuality> scalingModes = {
	VideoScaleQuality::LINEAR,
	VideoScaleQuality::NEAR_PERFECT,
	VideoScaleQuality::PERFECT,
};

Launcher::Launcher(int argc, char* argv[]) : StracciatellaLauncher() {
	this->argc = argc;
	this->argv = argv;
	this->exePath;
	this->engine_options = nullptr;
}

Launcher::~Launcher() {
	if (this->engine_options) {
		free_engine_options(this->engine_options);
		this->engine_options = nullptr;
	}
}

void Launcher::loadJa2Json() {
	char* rustExePath = find_ja2_executable(argv[0]);
	this->exePath = std::string(rustExePath);
	free_rust_string(rustExePath);

	if (this->engine_options) {
		free_engine_options(this->engine_options);
		this->engine_options = nullptr;
	}
	this->engine_options = create_engine_options(argv, argc);

	if (this->engine_options == NULL) {
		exit(EXIT_FAILURE);
	}
	if (should_show_help(this->engine_options)) {
		exit(EXIT_SUCCESS);
	}
}

void Launcher::show() {
	editorButton->callback( (Fl_Callback*)startEditor, (void*)(this) );
	playButton->callback( (Fl_Callback*)startGame, (void*)(this) );
	gameDirectoryInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	browseJa2DirectoryButton->callback((Fl_Callback *) openGameDirectorySelector, (void *) (this));
	gameVersionInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	guessVersionButton->callback( (Fl_Callback*)guessVersion, (void*)(this) );
	scalingModeChoice->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	resolutionXInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	resolutionYInput->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	auto game_json_path = get_game_json_path();
	gameSettingsOutput->value(game_json_path);
	free_rust_string(game_json_path);
	fullscreenCheckbox->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	playSoundsCheckbox->callback( (Fl_Callback*)widgetChanged, (void*)(this) );
	auto ja2_json_path = find_path_from_stracciatella_home("ja2.json", false);
	if (ja2_json_path) {
		ja2JsonPathOutput->value(ja2_json_path);
		free_rust_string(ja2_json_path);
	} else {
		ja2JsonPathOutput->value("failed to find path to ja2.json");
	}
	ja2JsonReloadBtn->callback( (Fl_Callback*)reloadJa2Json, (void*)(this) );
	ja2JsonSaveBtn->callback( (Fl_Callback*)saveJa2Json, (void*)(this) );

	populateChoices();
	initializeInputsFromDefaults();

	const Fl_PNG_Image icon("logo32.png", logo32_png, 1374);
	stracciatellaLauncher->icon(&icon);
	stracciatellaLauncher->show();
}

void Launcher::initializeInputsFromDefaults() {
	char* rustResRootPath = get_vanilla_game_dir(this->engine_options);
	gameDirectoryInput->value(rustResRootPath);
	free_rust_string(rustResRootPath);

	auto rustResVersion = get_resource_version(this->engine_options);
	auto resourceVersionIndex = 0;
	for (auto version : predefinedVersions) {
		if (version == rustResVersion) {
			break;
		}
		resourceVersionIndex += 1;
	}
	gameVersionInput->value(resourceVersionIndex);

	int x = get_resolution_x(this->engine_options);
	int y = get_resolution_y(this->engine_options);

	resolutionXInput->value(x);
	resolutionYInput->value(y);

	VideoScaleQuality quality = get_scaling_quality(this->engine_options);
	auto scalingModeIndex = 0;
	for (auto scalingMode : scalingModes) {
		if (scalingMode == quality) {
			break;
		}
		scalingModeIndex += 1;
	}
	this->scalingModeChoice->value(scalingModeIndex);

	fullscreenCheckbox->value(should_start_in_fullscreen(this->engine_options) ? 1 : 0);
	playSoundsCheckbox->value(should_start_without_sound(this->engine_options) ? 0 : 1);
	update(false, nullptr);
}

int Launcher::writeJsonFile() {
	set_start_in_fullscreen(this->engine_options, fullscreenCheckbox->value());
	set_start_without_sound(this->engine_options, !playSoundsCheckbox->value());

	set_vanilla_game_dir(this->engine_options, gameDirectoryInput->value());

	int x = (int)resolutionXInput->value();
	int y = (int)resolutionYInput->value();
	set_resolution(this->engine_options, x, y);

	auto currentResourceVersionIndex = gameVersionInput->value();
	auto currentResourceVersion = predefinedVersions.at(currentResourceVersionIndex);
	set_resource_version(this->engine_options, currentResourceVersion);

	auto currentScalingMode = scalingModes[this->scalingModeChoice->value()];
	set_scaling_quality(this->engine_options, currentScalingMode);

	bool success = write_engine_options(this->engine_options);

	if (success) {
		update(false, nullptr);
		SLOGD(LAUNCHER_TOPIC, "Succeeded writing config file");
		return 0;
	}
	SLOGD(LAUNCHER_TOPIC, "Failed writing config file");
	return 1;
}

void Launcher::populateChoices() {
	for(GameVersion version : predefinedVersions) {
		auto resourceVersionString = get_resource_version_string(version);
		gameVersionInput->add(resourceVersionString);
		free_rust_string(resourceVersionString);
    }
	for (auto res : predefinedResolutions) {
		char resolutionString[255];
		sprintf(resolutionString, "%dx%d", res.first, res.second);
		predefinedResolutionMenuButton->insert(-1, resolutionString, 0, setPredefinedResolution, this, 0);
	}

	for (auto scalingMode : scalingModes) {
		auto scalingModeString = get_scaling_quality_string(scalingMode);
		this->scalingModeChoice->add(scalingModeString);
		free_rust_string(scalingModeString);
	}
}

void Launcher::openGameDirectorySelector(Fl_Widget *btn, void *userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	Fl_Native_File_Chooser fnfc;
	fnfc.title("Select the original Jagged Alliance 2 install directory");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	fnfc.directory(window->gameDirectoryInput->value());

	switch ( fnfc.show() ) {
		case -1:
			break; // ERROR
		case  1:
			break; // CANCEL
		default:
			window->gameDirectoryInput->value(fnfc.filename());
			window->update(true, window->gameDirectoryInput);
			break; // FILE CHOSEN
	}
}

void Launcher::startExecutable(bool asEditor) {
	// check minimal resolution:
	if (resolutionIsInvalid()) {
		fl_message_title("Invalid resolution");
		fl_alert("Invalid custom resolution %dx%d.\nJA2 Stracciatella needs a resolution of at least 640x480.",
			(int) resolutionXInput->value(),
			(int) resolutionYInput->value());
		return;
	}

	std::string cmd("\"" + this->exePath + "\"");

	if (asEditor) {
		cmd += std::string(" -editor");
	}

	system(cmd.c_str());
}

bool Launcher::resolutionIsInvalid() {
	return resolutionXInput->value() < 640 || resolutionYInput->value() < 480;
}

void Launcher::update(bool changed, Fl_Widget *widget) {
	// invalid resolution warning
	if (resolutionIsInvalid()) {
		invalidResolutionLabel->show();
	} else {
		invalidResolutionLabel->hide();
	}

	// something changed indicator
	if (changed && ja2JsonPathOutput->value()[0] != '*') {
		std::string tmp("*"); // add '*'
		tmp += ja2JsonPathOutput->value();
		ja2JsonPathOutput->value(tmp.c_str());
	} else if (!changed && ja2JsonPathOutput->value()[0] == '*') {
		std::string tmp(ja2JsonPathOutput->value() + 1); // remove '*'
		ja2JsonPathOutput->value(tmp.c_str());
	}
}

void Launcher::startGame(Fl_Widget* btn, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	window->writeJsonFile();
	if (!check_if_relative_path_exists(window->gameDirectoryInput->value(), "Data", true)) {
		fl_message_title(window->playButton->label());
		auto choice = fl_choice("Data dir not found.\nAre you sure you want to continue?", "Stop", "Continue", 0);
		if (choice != 1) {
			return;
		}
	}
	window->startExecutable(false);
}

void Launcher::startEditor(Fl_Widget* btn, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );

	window->writeJsonFile();
	if (!check_if_relative_path_exists(window->gameDirectoryInput->value(), "Data/Editor.slf", true)) {
		fl_message_title(window->editorButton->label());
		auto choice = fl_choice("Editor.slf not found.\nAre you sure you want to continue?", "Stop", "Continue", 0);
		if (choice != 1) {
			return;
		}
	}
	window->startExecutable(true);
}

void Launcher::guessVersion(Fl_Widget* btn, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	fl_message_title("Guess Game Version");
	auto choice = fl_choice("Comparing resources packs can take a long time.\nAre you sure you want to continue?", "Stop", "Continue", 0);
	if (choice != 1) {
		return;
	}

	char* log = NULL;
	auto gamedir = window->gameDirectoryInput->value();
	auto guessedVersion = guess_resource_version(gamedir, &log);
	printf("%s", log);
	if (guessedVersion != -1) {
		auto resourceVersionIndex = 0;
		for (auto version : predefinedVersions) {
			if (version == (VanillaVersion) guessedVersion) {
				break;
			}
			resourceVersionIndex += 1;
		}
		window->gameVersionInput->value(resourceVersionIndex);
		window->update(true, window->gameVersionInput);
		fl_message_title(window->guessVersionButton->label());
		fl_message("Success!");
	} else {
		fl_message_title(window->guessVersionButton->label());
		fl_alert("Failure!");
	}
	free_rust_string(log);
}

void Launcher::setPredefinedResolution(Fl_Widget* btn, void* userdata) {
	Fl_Menu_Button* menuBtn = static_cast< Fl_Menu_Button* >( btn );
	Launcher* window = static_cast< Launcher* >( userdata );
	std::string res = menuBtn->mvalue()->label();
	int split_index = res.find(RESOLUTION_SEPARATOR);
	int x = atoi(res.substr(0, split_index).c_str());
	int y = atoi(res.substr(split_index+1, res.length()).c_str());
	window->resolutionXInput->value(x);
	window->resolutionYInput->value(y);
	window->update(true, btn);
}

void Launcher::widgetChanged(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	window->update(true, widget);
}

void Launcher::reloadJa2Json(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	window->loadJa2Json();
	window->initializeInputsFromDefaults();
}

void Launcher::saveJa2Json(Fl_Widget* widget, void* userdata) {
	Launcher* window = static_cast< Launcher* >( userdata );
	window->writeJsonFile();
}
