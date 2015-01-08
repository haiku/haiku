/* Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT license.
 */


#include <stdlib.h>
#include <vector>

#include <Application.h>
#include <Box.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <MediaDefs.h>
#include <OptionPopUp.h>
#include <TextControl.h>
#include <Window.h>

#include <Interpolate.h>
#include <Resampler.h>


static const int32 kMsgParametersChanged = 'pmch';


// #pragma mark -


class Wave
{
	public:
				Wave()
					:
					fData(NULL)
				{ Resize(1); step = 1; }
		float	ValueAt(int index) { return fData[index % fSize]; }
		void	SetValue(int pos, float val) { fData[pos] = val; }
		void	Resize(int newSize) {
			if (newSize <= 0)
				newSize = 1;
			fData = (float*)realloc(fData, newSize * sizeof(float));
			fSize = newSize;
		}
		int		Size() { return fSize; }
		void*	Raw() { return fData; }

		float step;
	private:
		float* fData;
		int fSize;
};


// #pragma mark -


class WaveView: public BView
{
	public:
				WaveView();

		void	Draw(BRect update);

		Wave	waves[3]; // reference, input, and output
		float	zoom;
};


WaveView::WaveView()
	:
	BView("wave", B_WILL_DRAW)
{
	SetExplicitMinSize(BSize(512, 256));
}


void
WaveView::Draw(BRect update)
{
	SetOrigin(0, 256);
	SetPenSize(1);
	for (float i = update.left; i <= update.right; i++) {
		if (i < waves[0].Size())
			SetHighColor(make_color(0, 0, 0, 255));
		else
			SetHighColor(make_color(180, 180, 180, 255));
		BPoint p(i, waves[0].ValueAt(i) * zoom);
		StrokeLine(p, p);
	}

	float i = 0;
	float w1pos = 0;

	// Skip the part outside the updat rect
	while (w1pos <= update.left) {
		w1pos += waves[1].step;
		i++;
	}

	while (w1pos <= update.right) {
		if (i < waves[1].Size())
			SetHighColor(make_color(255, 0, 0, 255));
		else
			SetHighColor(make_color(255, 180, 180, 255));
		BPoint p1(w1pos, INT16_MIN);
		BPoint p2(w1pos, waves[1].ValueAt(i) * zoom);
		StrokeLine(p1, p2);

		w1pos += waves[1].step;
		i++;
	}

	i = 0;
	w1pos = 0;

	// Skip the part outside the updat rect
	while (w1pos <= update.left) {
		w1pos += waves[2].step;
		i++;
	}

	while (w1pos <= update.right) {
		if (i < waves[2].Size())
			SetHighColor(make_color(0, 255, 0, 255));
		else
			SetHighColor(make_color(180, 255, 180, 255));
		BPoint p1(w1pos, INT16_MAX);
		BPoint p2(w1pos, waves[2].ValueAt(i) * zoom);
		StrokeLine(p1, p2);

		w1pos += waves[2].step;
		i++;
	}

}


// #pragma mark -


static BOptionPopUp* makeFormatMenu()
{
	BOptionPopUp* format = new BOptionPopUp("fmt", "Sample format:", NULL);
	format->AddOptionAt("U8", media_raw_audio_format::B_AUDIO_UCHAR, 0);
	format->AddOptionAt("S8", media_raw_audio_format::B_AUDIO_CHAR, 1);
	format->AddOptionAt("S16", media_raw_audio_format::B_AUDIO_SHORT, 2);
	format->AddOptionAt("S32", media_raw_audio_format::B_AUDIO_INT, 3);
	format->AddOptionAt("F32", media_raw_audio_format::B_AUDIO_FLOAT, 4);

	return format;
}


class MainWindow: public BWindow
{
	public:
				MainWindow();

		void	MessageReceived(BMessage* what);

	private:
		BTextControl*	fInputRate;
		BTextControl*	fOutputRate;
		BCheckBox*		fInterpolate;

		BTextControl*	fSignalVolume;
		BTextControl*	fSignalFrequency;

		WaveView*		fWaveView;
};


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 400, 400), "Mixer test", B_DOCUMENT_WINDOW,
		B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BBox* inputGroup = new BBox("Input", 0, B_FANCY_BORDER);
	inputGroup->SetLabel("Input");

	BGroupView* inputs = new BGroupView(B_VERTICAL);
	inputs->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	inputGroup->AddChild(inputs);

	fInputRate = new BTextControl("rate", "Sampling rate:", "256",
		new BMessage(kMsgParametersChanged));
	inputs->AddChild(fInputRate);
#if 0
	inputs->AddChild(makeFormatMenu());
#endif

	fSignalVolume = new BTextControl("vol", "Volume:", "127",
		new BMessage(kMsgParametersChanged));
	inputs->AddChild(fSignalVolume);
	fSignalFrequency = new BTextControl("freq", "Signal freq:", "256",
		new BMessage(kMsgParametersChanged));
	inputs->AddChild(fSignalFrequency);

	BBox* outputGroup = new BBox("Output", 0, B_FANCY_BORDER);
	outputGroup->SetLabel("Output");

	BGroupView* outputs = new BGroupView(B_VERTICAL);
	outputs->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	outputGroup->AddChild(outputs);

	fOutputRate = new BTextControl("rate", "Sampling rate:", "256",
		new BMessage(kMsgParametersChanged));
	outputs->AddChild(fOutputRate);
#if 0
	outputs->AddChild(makeFormatMenu());
	BTextControl* volume = new BTextControl("vol", "Gain:", "1", NULL);
	outputs->AddChild(volume);
#endif

	fInterpolate = new BCheckBox("interp", "Interpolate",
		new BMessage(kMsgParametersChanged));
	outputs->AddChild(fInterpolate);

	BGroupView* header = new BGroupView(B_HORIZONTAL);
	AddChild(header);
	header->GroupLayout()->SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
		B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING);
	header->AddChild(inputGroup);
	header->AddChild(outputGroup);

	AddChild(fWaveView = new WaveView());
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgParametersChanged:
		{
			int freq = atoi(fSignalFrequency->Text());
			fWaveView->waves[0].Resize(freq);

			int irate = atoi(fInputRate->Text());
			fWaveView->waves[1].Resize(irate);

			int orate = atoi(fOutputRate->Text());
			fWaveView->waves[2].Resize(orate);

			int vol = atoi(fSignalVolume->Text());

			fWaveView->waves[0].step = 1;
			fWaveView->waves[1].step = (float)freq / irate;
			fWaveView->waves[2].step = (float)freq / orate;
			fWaveView->zoom = vol;

			for (int i = 0; i < freq; i++) {
				fWaveView->waves[0].SetValue(i, sinf(i * 2 * M_PI / freq));
			}

			for (int i = 0; i < irate; i++) {
				fWaveView->waves[1].SetValue(i,
					fWaveView->waves[0].ValueAt(i * freq / irate));
			}

			// FIXME handle gain
			if (fInterpolate->Value() == B_CONTROL_ON) {
				Interpolate sampler(media_raw_audio_format::B_AUDIO_FLOAT,
					media_raw_audio_format::B_AUDIO_FLOAT);

				// First call initializes the "old sample" in the interpolator.
				// Since we do the interpolation on exactly one period of the
				// sound wave, this works.
				sampler.Resample(fWaveView->waves[1].Raw(), sizeof(float), irate,
					fWaveView->waves[2].Raw(), sizeof(float), orate, 1);

				sampler.Resample(fWaveView->waves[1].Raw(), sizeof(float), irate,
					fWaveView->waves[2].Raw(), sizeof(float), orate, 1);
			} else {
				Resampler sampler(media_raw_audio_format::B_AUDIO_FLOAT,
					media_raw_audio_format::B_AUDIO_FLOAT);

				sampler.Resample(fWaveView->waves[1].Raw(), sizeof(float), irate,
					fWaveView->waves[2].Raw(), sizeof(float), orate, 1);
			}

			fWaveView->Invalidate();
			return;
		}
	}

	BWindow::MessageReceived(message);
}


int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-MixerToy");
	(new MainWindow())->Show();
	app.Run();
}
