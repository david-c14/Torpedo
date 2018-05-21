/******************************************************
**
** Reference example for sending and receiving using
** the PatchInputPort and PatchOutputPort objects
** which implement the PTCH protocol.
**
** In this demo we have 
**
** 	completed method
**	received method
**	error method
**	send method
**	isBusy method
**
**	checking the message type (pluginName and moduleName)
**
*******************************************************/


#include "TorpedoDemo.hpp"
#include <mutex>
#include "dsp/digital.hpp"
				// torpedo.hpp is the only include necessary 
				// to use torpedo. Your project should also
				// compile and link torpedo.cpp
#include "torpedo.hpp"

struct TorPatch;		// Forward declarations, because I want to
struct TorPatchNano;		// include a reference to the module in 
				// the ports.

	// 
	// I'm subclassing the PatchOutpuPort only because I want to
	// use the completed method to light a little green light.
	// Otherwise I could use the PatchOutputPort directly
	//
struct TorPatchOutputPort : Torpedo::PatchOutputPort {
	TorPatch *tpModule;
	TorPatchOutputPort(TorPatch *module, unsigned int portNum):Torpedo::PatchOutputPort((Module *)module, portNum) {tpModule = module;};
	void completed() override;
};

	//
	// I have to subclass the PatchInputPort so that I can override the
	// received method to actually get at received messages
	//
struct TorPatchInputPort : Torpedo::PatchInputPort {
	TorPatch *tpModule;
	TorPatchInputPort(TorPatch *module, unsigned int portNum):Torpedo::PatchInputPort((Module *)module, portNum) {tpModule = module;};
	void received(std::string pluginName, std::string moduleName, json_t *rootJ) override;
	void error(unsigned int errorType) override;
};

struct TorPatch : Module  {
	enum ParamIds {
		PARAM_1,		// The first knob parameter
		PARAM_2,		// The second knob parameter
		PARAM_3,		// The red/yellow/green light switch
		PARAM_SEND,		// Push button to send message
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_TOR,		// An input port for torpedo to use
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_TOR,		// An output port for torpedo to use
		OUTPUT_V1,		// The first knob output
		OUTPUT_V2,		// The second knob output
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_GREEN,		// The big green/yellow/red light
		LIGHT_RED,		// The big green/yellow/red light
		LIGHT_COMPLETE,		// The tiny completed light
		LIGHT_ERROR,		// The tiny error light
		LIGHT_RECEIVE,		// The tiny message receive light
		NUM_LIGHTS
	};

	std::mutex mtx;			// Mutex for locking between 
					// module and widget

	float v1;			// These hold received param values
	float v2;			// so that the widget can get them
	float v3;

	int toSend = 1;			// A flag to say we have changes to send

	int isDirty = 0;		// A flag to say we have received
					// changes.

	int hasWidget = 0;		// This module is not headless

	PulseGenerator complete;	// These are only used to keep the
	PulseGenerator error;		// tiny lights lit for 1/10 second.
	PulseGenerator receive;
	SchmittTrigger sendTrigger;	// Detect send button

		// Wrap a Torpedo output port around a regular output port
	TorPatchOutputPort outPort = TorPatchOutputPort(this, OUTPUT_TOR);

		// Wrap a Torpedo input port around a regular input port
	TorPatchInputPort inPort = TorPatchInputPort(this, INPUT_TOR);

	TorPatch() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	void step() override;
};

void TorPatch::step() {
		//
		// Detect parameter changes so we don't send messages
		// continuously.  This is optional. We could send messages
		// every time the port is not busy, or we could use the
		// queue semantics of the port and just try to send a 
		// message every step.
		//
	if (sendTrigger.process(params[PARAM_SEND].value))
		toSend = 1;
	if (outputs[OUTPUT_V1].value != params[PARAM_1].value)
		toSend = 1;
	outputs[OUTPUT_V1].value = params[PARAM_1].value;
	if (outputs[OUTPUT_V2].value != params[PARAM_2].value)
		toSend = 1;
	outputs[OUTPUT_V2].value = params[PARAM_2].value;
	if (lights[LIGHT_RED].value != (params[PARAM_3].value > 0.5f))
		toSend = 1;
	lights[LIGHT_RED].value = (params[PARAM_3].value > 0.5f);
	if (lights[LIGHT_GREEN].value != (params[PARAM_3].value < 1.5f))
		toSend = 1;
	lights[LIGHT_GREEN].value = (params[PARAM_3].value < 1.5f);

		//
		// Here we use the pulse generators to keep the lights lit
		// for 1/10 second.
		//
	lights[LIGHT_RECEIVE].value = receive.process(engineGetSampleTime());
	lights[LIGHT_ERROR].value = error.process(engineGetSampleTime());
	lights[LIGHT_COMPLETE].value = complete.process(engineGetSampleTime());

		//
		// If we have some changes to send, and the port is not 
		// already mid-message, construct a json object and send it
		// The plugin slug is a good way to control the identity
		// of your message, but the moduleName parameter does not
		// need to be an actual module name; it's more a way of 
		// distinguishing one message type from another
		//
	if (toSend && !outPort.isBusy()) {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "param1", json_real(params[PARAM_1].value));
		json_object_set_new(rootJ, "param2", json_real(params[PARAM_2].value));
		json_object_set_new(rootJ, "param3", json_real(params[PARAM_3].value));
		outPort.send(std::string(TOSTRING(SLUG)), std::string("TorPatch"), rootJ);
		toSend = 0;
	}

		//
		// The torpedo ports must be processed every step if they are
		// not going to miss anything
		//
	outPort.process();
	inPort.process();
}

	//
	// This received method is called whenever the PatchInputPort receives
	// a message using the PTCH protocol. It will extract the pluginName,
	// moduleName and the patch json from the message and pass it in here
	//
	// In this method I'm rejecting any message of a type I don't want
	// to process.
	// Set the tiny received light
	// Place the received parameters into the module
	// If the module is headless, update the parameters directly.
	// Otherwise the isDirty flag will signal the moduleWidget to update.
	//
void TorPatchInputPort::received(std::string pluginName, std::string moduleName, json_t *rootJ) {

	if (pluginName.compare(TOSTRING(SLUG))) return;
	if (moduleName.compare("TorPatch")) return;

	tpModule->receive.trigger(0.1f);

	float v1 = 0.0f;
	float v2 = 0.0f;
	float v3 = 0.0f;
	int s1 = 0;
	int s2 = 0;
	int s3 = 0;

	json_t *j1 = json_object_get(rootJ, "param1");
	if (j1) {
		v1 = json_number_value(j1);
		s1 = 1;
	}
	json_t *j2 = json_object_get(rootJ, "param2");
	if (j2) {
		v2 = json_number_value(j2);
		s2 = 1;
	}
	json_t *j3 = json_object_get(rootJ, "param3");
	if (j3) {
		v3 = json_number_value(j3);
		s3 = 1;
	}
	
	// Lock using the mutex
	{
		std::lock_guard<std::mutex> guard(tpModule->mtx);
		if (s1)
			tpModule->v1 = v1;
		if (s2)
			tpModule->v2 = v2;
		if (s3)
			tpModule->v3 = v3;
		tpModule->isDirty = 1;
	}

	if (!tpModule->hasWidget) {
		engineSetParam(tpModule, TorPatch::PARAM_1, tpModule->v1);
		engineSetParam(tpModule, TorPatch::PARAM_2, tpModule->v2);
		engineSetParam(tpModule, TorPatch::PARAM_3, tpModule->v3);
	}
}

	//
	// If the input port receives a broken message it will call error
	// I'm overriding that to set the tiny red error light
	//
void TorPatchInputPort::error(unsigned int errorType) {
	tpModule->error.trigger(0.1f);
}

	//
	// Once the output port has finished sending a message it will
	// call the completed method. I'm overriding that to set a light
	//
void TorPatchOutputPort::completed() {
	tpModule->complete.trigger(0.1f);
}

	//
	// The module widget step method checks the module's isDirty flag
	// and if it needs to it updates the knobs and switches
	//
struct TorPatchWidget : ModuleWidget {

	TorPatch *tpModule;

	ParamWidget *p1;
	ParamWidget *p2;
	ParamWidget *p3;

	TorPatchWidget(TorPatch *module) : ModuleWidget(module) {
		tpModule = module;
		setPanel(SVG::load(assetPlugin(plugin, "res/TorPatch.svg")));

		addInput(Port::create<sub_port_black>(Vec(4,19), Port::INPUT, module, TorPatch::INPUT_TOR));

		addOutput(Port::create<sub_port_black>(Vec(92,19), Port::OUTPUT, module, TorPatch::OUTPUT_TOR));

		addOutput(Port::create<sub_port>(Vec(4,149), Port::OUTPUT, module, TorPatch::OUTPUT_V1));
		addOutput(Port::create<sub_port>(Vec(92,149), Port::OUTPUT, module, TorPatch::OUTPUT_V2));

		p1 = ParamWidget::create<sub_knob_med>(Vec(4, 105), module, TorPatch::PARAM_1, -5.0f, 5.0f, 0.0f);
		addParam(p1);
		p2 = ParamWidget::create<sub_knob_med>(Vec(79, 105), module, TorPatch::PARAM_2, -5.0f, 5.0f, 0.0f);
		addParam(p2);
		p3 = ParamWidget::create<sub_sw_3>(Vec(53, 265), module, TorPatch::PARAM_3, 0.0f, 2.0f, 0.0f);
		addParam(p3);
		addParam(ParamWidget::create<CKD6>(Vec(50, 19), module, TorPatch::PARAM_SEND, 0.0f, 1.0f, 0.0f));

		addChild(ModuleLightWidget::create<LargeLight<GreenRedLight>>(Vec(52, 300), module, TorPatch::LIGHT_GREEN));
		addChild(ModuleLightWidget::create<TinyLight<GreenLight>>(Vec(4, 45), module, TorPatch::LIGHT_RECEIVE));
		addChild(ModuleLightWidget::create<TinyLight<RedLight>>(Vec(26, 45), module, TorPatch::LIGHT_ERROR));
		addChild(ModuleLightWidget::create<TinyLight<GreenLight>>(Vec(114, 45), module, TorPatch::LIGHT_COMPLETE));
		tpModule->hasWidget = 1;
	}

	//
	// In this module widget step function we use a mutex to 
	// prevent race conditions between the module and the widget.
	// Because they are running in different threads.
	// We do the bare minimum we can inside the guarded section
	void step() override {
		float v1 = 0.0f;
		float v2 = 0.0f;
		float v3 = 0.0f;
		int isDirty = 0;

		// Lock using the mutex
		{
			std::lock_guard<std::mutex> guard(tpModule->mtx);

			if (tpModule->isDirty) {
				v1 = tpModule->v1;
				v2 = tpModule->v2;
				v3 = tpModule->v3;
				tpModule->isDirty = 0;
				isDirty = 1;
			}
		}

		if (isDirty) {
			p1->setValue(v1);
			p2->setValue(v2);
			p3->setValue(v3);
		}
		ModuleWidget::step();
	}
};

	//
	// I have to subclass the PatchInputPort so that I can override the
	// received method to actually get at received messages
	//
struct TorPatchNanoInputPort : Torpedo::PatchInputPort {
	TorPatchNano *tpModule;
	TorPatchNanoInputPort(TorPatchNano *module, unsigned int portNum):Torpedo::PatchInputPort((Module *)module, portNum) {tpModule = module;};
	void received(std::string pluginName, std::string moduleName, json_t *rootJ) override;
	void error(unsigned int errorType) override;
};

//
// A cut down TorPatch module with no knobs or torpedo output port.
//
struct TorPatchNano : Module  {
	enum ParamIds {
		PARAM_1,		// The first knob parameter
		PARAM_2,		// The second knob parameter
		PARAM_3,		// The red/yellow/green light switch
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_TOR,		// An input port for torpedo to use
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_V1,		// The first knob output
		OUTPUT_V2,		// The second knob output
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_GREEN,		// The big green/yellow/red light
		LIGHT_RED,		// The big green/yellow/red light
		LIGHT_ERROR,		// The tiny error light
		LIGHT_RECEIVE,		// The tiny message receive light
		NUM_LIGHTS
	};

	PulseGenerator error;		// tiny lights lit for 1/10 second.
	PulseGenerator receive;

		// Wrap a Torpedo input port around a regular input port
	TorPatchNanoInputPort inPort = TorPatchNanoInputPort(this, INPUT_TOR);

	TorPatchNano() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	void step() override;
};

void TorPatchNano::step() {
		//
		// Update outputs and the red/green light.
		//
	outputs[OUTPUT_V1].value = params[PARAM_1].value;
	outputs[OUTPUT_V2].value = params[PARAM_2].value;
	lights[LIGHT_RED].value = (params[PARAM_3].value > 0.5f);
	lights[LIGHT_GREEN].value = (params[PARAM_3].value < 1.5f);

		//
		// Here we use the pulse generators to keep the lights lit
		// for 1/10 second.
		//
	lights[LIGHT_ERROR].value = error.process(engineGetSampleTime());
	lights[LIGHT_RECEIVE].value = receive.process(engineGetSampleTime());

		//
		// The torpedo ports must be processed every step if they are
		// not going to miss anything
		//
	inPort.process();
}

	//
	// This received method is called whenever the PatchInputPort receives
	// a message using the PTCH protocol. It will extract the pluginName,
	// moduleName and the patch json from the message and pass it in here
	//
	// In this method I'm rejecting any message of a type I don't want
	// to process.
	// Set the tiny received light
	// Place the received parameters into the module
	//
void TorPatchNanoInputPort::received(std::string pluginName, std::string moduleName, json_t *rootJ) {

	if (pluginName.compare(TOSTRING(SLUG))) return;
	if (moduleName.compare("TorPatch")) return;

	tpModule->receive.trigger(0.1f);

	json_t *j1 = json_object_get(rootJ, "param1");
	if (j1) {
		engineSetParam(tpModule, TorPatchNano::PARAM_1, json_number_value(j1));
	}
	json_t *j2 = json_object_get(rootJ, "param2");
	if (j2) {
		engineSetParam(tpModule, TorPatchNano::PARAM_1, json_number_value(j2));
	}
	json_t *j3 = json_object_get(rootJ, "param3");
	if (j3) {
		engineSetParam(tpModule, TorPatchNano::PARAM_1, json_number_value(j3));
	}
}

	//
	// If the input port receives a broken message it will call error
	// I'm overriding that to set the tiny red error light
	//
void TorPatchNanoInputPort::error(unsigned int errorType) {
	tpModule->error.trigger(0.1f);
}

struct TorPatchNanoWidget : ModuleWidget {

	TorPatchNano *tpModule;

	TorPatchNanoWidget(TorPatchNano *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/TorPatch.svg")));

		addOutput(Port::create<sub_port>(Vec(4,149), Port::OUTPUT, module, TorPatch::OUTPUT_V1));
		addOutput(Port::create<sub_port>(Vec(92,149), Port::OUTPUT, module, TorPatch::OUTPUT_V2));

		addChild(ModuleLightWidget::create<LargeLight<GreenRedLight>>(Vec(52, 300), module, TorPatch::LIGHT_GREEN));
		addChild(ModuleLightWidget::create<TinyLight<GreenLight>>(Vec(4, 45), module, TorPatch::LIGHT_RECEIVE));
		addChild(ModuleLightWidget::create<TinyLight<RedLight>>(Vec(26, 45), module, TorPatch::LIGHT_ERROR));
	}
};

Model *modelTorPatch = Model::create<TorPatch, TorPatchWidget>("TorpedoDemo", "Torpedo Patch Demo", "Torpedo Patch Demo", LOGIC_TAG);
Model *modelTorPatchNano = Model::create<TorPatchNano, TorPatchNanoWidget>("TorpedoDemo", "Torpedo Patch Nano Demo", "Torpedo Patch Demo Cut Down Module", LOGIC_TAG);
