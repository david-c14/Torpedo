#include "TorpedoDemo.hpp"
#include "torpedo.hpp"

struct TorNotes;

struct TorNotesInput : Torpedo::PatchInputPort {
	TorNotes *tnModule;
	TorNotesInput(TorNotes *module, unsigned int portNum) : Torpedo::PatchInputPort((Module *)module, portNum) { tnModule = module; }
	void received(std::string pluginName, std::string moduleName, json_t *rootJ) override;
};

struct TorNotes : Module {
	TorNotesInput inPort = TorNotesInput(this, 0);
	Torpedo::PatchOutputPort outPort = Torpedo::PatchOutputPort(this, 0);
	TorNotes() : Module (0, 1, 1, 0) {}
	void step() override {
		inPort.process();
		outPort.process();
	}
	void sendText(std::string text) {
		json_t *rootJ = json_object();;

		// text
		json_object_set_new(rootJ, "text", json_string(text.c_str()));

		outPort.send("TorpedoDemo", "TorNotesText", rootJ); 
	}
	std::string text;
	int isDirty = false;
};

struct TorNotesText : LedDisplayTextField {
	TorNotes *tnModule;
	void onTextChange() override {
		LedDisplayTextField::onTextChange();
		tnModule->sendText(text);
	}
};

void TorNotesInput::received(std::string pluginName, std::string moduleName, json_t *rootJ) {
	if (pluginName.compare("TorpedoDemo")) return;
	if (moduleName.compare("TorNotesText")) return;
	json_t *text = json_object_get(rootJ, "text");
	if (text) {
		tnModule->text.assign(json_string_value(text));
		tnModule->isDirty = true;
	}
}

struct TorNotesWidget : ModuleWidget {
	TorNotesText *textField;

	TorNotesWidget(TorNotes *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/TorNotes.svg")));

		addInput(Port::create<sub_port_black>(Vec(4,19), Port::INPUT, module, 0));
		addOutput(Port::create<sub_port_black>(Vec(211,19), Port::OUTPUT, module, 0));	

		textField = Widget::create<TorNotesText>(mm2px(Vec(3.39962, 15.8373)));
		textField->box.size = mm2px(Vec(74.480, 100.753));
		textField->multiline = true;
		textField->tnModule = module;
		addChild(textField);
	}

	json_t *toJson() override {
		json_t *rootJ = ModuleWidget::toJson();

		// text
		json_object_set_new(rootJ, "text", json_string(textField->text.c_str()));

		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		ModuleWidget::fromJson(rootJ);

		// text
		json_t *textJ = json_object_get(rootJ, "text");
		if (textJ)
			textField->text = json_string_value(textJ);
	}

	void step() override {
		TorNotes *tnModule = dynamic_cast<TorNotes *>(module);
		if (tnModule->isDirty) {
			textField->text = tnModule->text;
			tnModule->isDirty = false;
		}
		ModuleWidget::step();
	}
};


Model *modelTorNotes = Model::create<TorNotes, TorNotesWidget>("TorpedoDemo", "Torpedo Notes Demo", "Torpedo Notes Demo", BLANK_TAG);
