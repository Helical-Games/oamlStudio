#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oamlCommon.h"
#include "tinyxml2.h"

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/scrolbar.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/filename.h>


IMPLEMENT_APP(oamlStudio)

wxDEFINE_EVENT(EVENT_ADD_AUDIO, wxCommandEvent);
wxDEFINE_EVENT(EVENT_REMOVE_AUDIO, wxCommandEvent);
wxDEFINE_EVENT(EVENT_SELECT_AUDIO, wxCommandEvent);

oamlApi *oaml;

oamlTrackInfo* GetTrackInfo(std::string trackName) {
	oamlTracksInfo* info = oaml->GetTracksInfo();
	if (info == NULL)
		return NULL;

	for (size_t i=0; i<info->tracks.size(); i++) {
		if (info->tracks[i].name == trackName) {
			return &info->tracks[i];
		}
	}

	return NULL;
}

oamlAudioInfo* GetAudioInfo(std::string trackName, std::string audioFile) {
	oamlTracksInfo* info = oaml->GetTracksInfo();
	for (size_t i=0; i<info->tracks.size(); i++) {
		if (info->tracks[i].name == trackName) {
			for (size_t j=0; j<info->tracks[i].audios.size(); j++) {
				if (info->tracks[i].audios[j].filename == audioFile) {
					return &info->tracks[i].audios[j];
				}
			}

			return NULL;
		}
	}
	return NULL;
}

void AddAudioInfo(std::string trackName, oamlAudioInfo& audio) {
	oamlTrackInfo* info = GetTrackInfo(trackName);
	if (info == NULL)
		return;

	info->audios.push_back(audio);
}

void RemoveAudioInfo(std::string trackName, std::string audioFile) {
	oamlTracksInfo* info = oaml->GetTracksInfo();
	for (size_t i=0; i<info->tracks.size(); i++) {
		if (info->tracks[i].name == trackName) {
			for (size_t j=0; j<info->tracks[i].audios.size(); j++) {
				if (info->tracks[i].audios[j].filename == audioFile) {
					info->tracks[i].audios.erase(info->tracks[i].audios.begin()+j);
					return;
				}
			}

			return;
		}
	}
}


class AudioPanel : public wxPanel {
private:
	wxBoxSizer *sizer;
	std::vector<WaveformDisplay*> waveDisplays;
	std::string trackName;
	int panelIndex;

public:
	AudioPanel(wxFrame* parent, int index, std::string name) : wxPanel(parent) {
		wxString texts[4] = { "Intro", "Main loop", "Conditional", "Ending" };

		panelIndex = index;
		trackName = name;

		sizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *staticText = new wxStaticText(this, wxID_ANY, texts[index], wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
		staticText->SetBackgroundColour(wxColour(0xD0, 0xD0, 0xD0));
		sizer->Add(staticText, 0, wxALL | wxEXPAND | wxGROW, 5);
		SetSizer(sizer);

		Bind(wxEVT_PAINT, &AudioPanel::OnPaint, this);
		Bind(wxEVT_RIGHT_UP, &AudioPanel::OnRightUp, this);
		Bind(wxEVT_COMMAND_MENU_SELECTED, &AudioPanel::OnMenuEvent, this, ID_AddAudio);
	}

	void OnPaint(wxPaintEvent& WXUNUSED(evt)) {
		wxPaintDC dc(this);

		wxSize size = GetSize();
		int x2 = size.GetWidth();
		int y2 = size.GetHeight();

		dc.SetPen(wxPen(wxColour(0, 0, 0), 4));
		dc.DrawLine(0,  0,  0,  y2);
		dc.DrawLine(x2, 0,  x2, y2);
		dc.DrawLine(0,  0,  x2, 0);
		dc.DrawLine(0,  y2, x2, y2);
	}

	void AddWaveform(oamlAudioInfo *audio, wxFrame *topWnd) {
		WaveformDisplay *waveDisplay = new WaveformDisplay((wxFrame*)this, topWnd);
		waveDisplay->SetSource(audio);

		waveDisplays.push_back(waveDisplay);

		sizer->Add(waveDisplay, 0, wxALL, 5);
		Layout();
	}

	void RemoveWaveform(std::string audioFile) {
		for (size_t i=0; i<waveDisplays.size(); i++) {
			WaveformDisplay *waveDisplay = waveDisplays[i];
			if (waveDisplay->GetAudioFile() == audioFile) {
				waveDisplays.erase(waveDisplays.begin()+i);
				waveDisplay->Destroy();
				break;
			}
		}

		Layout();
	}

	void AddAudio() {
		wxFileDialog openFileDialog(this, _("Open audio file"), ".", "", "Audio files (*.wav;*.aif;*.ogg)|*.aif;*.aiff;*.wav;*.wave;*.ogg", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;

		oamlAudioInfo audio;
		memset(&audio, 0, sizeof(oamlAudioInfo));
		wxFileName filename(openFileDialog.GetPath());
		wxFileName defsPath(oaml->GetDefsFile());
		filename.MakeRelativeTo(wxString(defsPath.GetPath()));
		audio.filename = filename.GetFullPath().ToStdString();
		switch (panelIndex) {
			case 0: audio.type = 1; break;
			case 1: audio.type = 2; break;
			case 2: audio.type = 4; break;
			case 3: audio.type = 3; break;
		}

		AddAudioInfo(trackName, audio);

		wxCommandEvent event(EVENT_ADD_AUDIO);
		event.SetString(audio.filename);
		wxPostEvent(GetParent(), event);
	}

	void OnMenuEvent(wxCommandEvent& event) {
		switch (event.GetId()) {
			case ID_AddAudio:
				AddAudio();
				break;
		}
	}

	void OnRightUp(wxMouseEvent& WXUNUSED(event)) {
		wxMenu menu(wxT(""));
		menu.Append(ID_AddAudio, wxT("&Add Audio"));
		PopupMenu(&menu);
	}
};

class ScrolledWidgetsPane : public wxScrolledWindow {
private:
	wxBoxSizer* sizer;
	AudioPanel* audioPanel[4];
	std::string trackName;

public:
	ScrolledWidgetsPane(wxWindow* parent, wxWindowID id, std::string name) : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL) {
		trackName = name;

		SetBackgroundColour(wxColour(0x40, 0x40, 0x40));
		SetScrollRate(50, 50);

		sizer = new wxBoxSizer(wxHORIZONTAL);

		for (int i=0; i<4; i++) {
			audioPanel[i] = new AudioPanel((wxFrame*)this, i, trackName);

			sizer->Add(audioPanel[i], 0, wxALL | wxEXPAND | wxGROW, 0);
		}

		SetSizer(sizer);
		Layout();

		sizer->Fit(this);
	}

	int GetPanelIndex(oamlAudioInfo *audio) {
		int i = 1;

		if (audio->type == 1) {
			i = 0;
		} else if (audio->type == 3) {
			i = 3;
		} else if (audio->type == 4) {
			i = 2;
		}
		return i;
	}

	void AddDisplay(std::string audioFile) {
		oamlAudioInfo *audio = GetAudioInfo(trackName, audioFile);
		if (audio == NULL)
			return;

		int i = GetPanelIndex(audio);
		audioPanel[i]->AddWaveform(audio, (wxFrame*)GetParent());

		SetSizer(sizer);
		Layout();
		sizer->Fit(this);
	}

	void RemoveDisplay(std::string audioFile) {
		oamlAudioInfo *audio = GetAudioInfo(trackName, audioFile);
		if (audio == NULL)
			return;

		int i = GetPanelIndex(audio);
		audioPanel[i]->RemoveWaveform(audioFile);

		RemoveAudioInfo(trackName, audioFile);

		SetSizer(sizer);
		Layout();
		sizer->Fit(this);
	}
};

class ControlPanel : public wxPanel {
private:
	wxTextCtrl *fileCtrl;
	wxTextCtrl *bpmCtrl;
	wxTextCtrl *bpbCtrl;
	wxTextCtrl *barsCtrl;
	wxTextCtrl *randomChanceCtrl;
	wxTextCtrl *minMovementBarsCtrl;
	wxTextCtrl *fadeInCtrl;
	wxTextCtrl *fadeOutCtrl;
	wxTextCtrl *xfadeInCtrl;
	wxTextCtrl *xfadeOutCtrl;
	wxGridSizer *sizer;
	std::string trackName;
	std::string audioFile;

public:
	ControlPanel(wxFrame* parent, wxWindowID id) : wxPanel(parent, id) {
		int ctrlWidth = 160;
		int ctrlHeight = -1;

		trackName = "";
		audioFile = "";

		sizer = new wxGridSizer(4, 0, 0);

		wxStaticText *staticText = new wxStaticText(this, wxID_ANY, wxString("Filename"));
		sizer->Add(staticText, 0, wxALL, 5);

		fileCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight), wxTE_READONLY);
		sizer->Add(fileCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Bpm"));
		sizer->Add(staticText, 0, wxALL, 5);

		bpmCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		bpmCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnBpmChange, this);
		sizer->Add(bpmCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Beats Per Bar"));
		sizer->Add(staticText, 0, wxALL, 5);

		bpbCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		bpbCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnBpbChange, this);
		sizer->Add(bpbCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Bars"));
		sizer->Add(staticText, 0, wxALL, 5);

		barsCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		barsCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnBarsChange, this);
		sizer->Add(barsCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Random Chance"));
		sizer->Add(staticText, 0, wxALL, 5);

		randomChanceCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		randomChanceCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnRandomChanceChange, this);
		sizer->Add(randomChanceCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Min movement bars"));
		sizer->Add(staticText, 0, wxALL, 5);

		minMovementBarsCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		minMovementBarsCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnMinMovementBarsChange, this);
		sizer->Add(minMovementBarsCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Fade In"));
		sizer->Add(staticText, 0, wxALL, 5);

		fadeInCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		fadeInCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnFadeInChange, this);
		sizer->Add(fadeInCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Fade Out"));
		sizer->Add(staticText, 0, wxALL, 5);

		fadeOutCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		fadeOutCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnFadeOutChange, this);
		sizer->Add(fadeOutCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Crossfade In"));
		sizer->Add(staticText, 0, wxALL, 5);

		xfadeInCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		xfadeInCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnXFadeInChange, this);
		sizer->Add(xfadeInCtrl, 0, wxALL, 5);

		staticText = new wxStaticText(this, wxID_ANY, wxString("Crossfade Out"));
		sizer->Add(staticText, 0, wxALL, 5);

		xfadeOutCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(ctrlWidth, ctrlHeight));
		xfadeOutCtrl->Bind(wxEVT_TEXT, &ControlPanel::OnXFadeOutChange, this);
		sizer->Add(xfadeOutCtrl, 0, wxALL, 5);

		SetSizer(sizer);
		SetMinSize(wxSize(-1, 180));

		Layout();
	}

	void OnBpmChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = bpmCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			double d = 0;
			str.ToDouble(&d);
			info->bpm = (float)d;
		}
	}

	void OnBpbChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = bpbCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->beatsPerBar = (int)l;
		}
	}

	void OnBarsChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = barsCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->bars = (int)l;
		}
	}

	void OnRandomChanceChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = randomChanceCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->randomChance = (int)l;
		}
	}

	void OnMinMovementBarsChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = minMovementBarsCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->minMovementBars = (int)l;
		}
	}

	void OnFadeInChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = fadeInCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->fadeIn = (int)l;
		}
	}

	void OnFadeOutChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = fadeOutCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->fadeOut = (int)l;
		}
	}

	void OnXFadeInChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = xfadeInCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->xfadeIn = (int)l;
		}
	}

	void OnXFadeOutChange(wxCommandEvent& WXUNUSED(event)) {
		wxString str = xfadeOutCtrl->GetLineText(0);
		if (str.IsEmpty())
			return;

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			long l = 0;
			str.ToLong(&l);
			info->xfadeOut = (int)l;
		}
	}

	void SetTrack(std::string name) {
		trackName = name;
	}

	void OnSelectAudio(std::string audio) {
		audioFile = audio;

		fileCtrl->Clear();
		bpmCtrl->Clear();
		bpbCtrl->Clear();
		barsCtrl->Clear();
		randomChanceCtrl->Clear();
		minMovementBarsCtrl->Clear();
		fadeInCtrl->Clear();
		fadeOutCtrl->Clear();
		xfadeInCtrl->Clear();
		xfadeOutCtrl->Clear();

		oamlAudioInfo *info = GetAudioInfo(trackName, audioFile);
		if (info) {
			*fileCtrl << info->filename;
			*bpmCtrl << info->bpm;
			*bpbCtrl << info->beatsPerBar;
			*barsCtrl << info->bars;
			*randomChanceCtrl << info->randomChance;
			*minMovementBarsCtrl << info->minMovementBars;
			*fadeInCtrl << info->fadeIn;
			*fadeOutCtrl << info->fadeOut;
			*xfadeInCtrl << info->xfadeIn;
			*xfadeOutCtrl << info->xfadeOut;
		}
	}
};

class StudioTimer : public wxTimer {
	wxWindow* pane;
public:
	StudioTimer(wxWindow* pane);

	void Notify();
};

StudioTimer::StudioTimer(wxWindow* pane) : wxTimer() {
	StudioTimer::pane = pane;
}

void StudioTimer::Notify() {
	pane->Layout();
}

class StudioFrame: public wxFrame {
private:
	std::string defsPath;
	wxListView* trackList;
	wxBoxSizer* mainSizer;
	wxBoxSizer* vSizer;
	ControlPanel* controlPane;
	ScrolledWidgetsPane* trackPane;
	StudioTimer* timer;

	oamlTracksInfo* tinfo;

	void AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, const char *value);
	void AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, int value);

	void SelectTrack(std::string name);

public:
	StudioFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

	void OnTrackListActivated(wxListEvent& event);
	void OnTrackListMenu(wxMouseEvent& event);
	void OnTrackEndLabelEdit(wxCommandEvent& event);
	void OnNew(wxCommandEvent& event);
	void OnLoad(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnSaveAs(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnAddTrack(wxCommandEvent& event);
	void OnEditTrackName(wxCommandEvent& event);
	void OnSelectAudio(wxCommandEvent& event);
	void OnAddAudio(wxCommandEvent& event);
	void OnRemoveAudio(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(StudioFrame, wxFrame)
	EVT_MENU(ID_New, StudioFrame::OnNew)
	EVT_MENU(ID_Load, StudioFrame::OnLoad)
	EVT_MENU(ID_Save, StudioFrame::OnSave)
	EVT_MENU(ID_SaveAs, StudioFrame::OnSaveAs)
	EVT_MENU(ID_Export, StudioFrame::OnExport)
	EVT_MENU(ID_Quit, StudioFrame::OnQuit)
	EVT_MENU(ID_About, StudioFrame::OnAbout)
	EVT_MENU(ID_AddTrack, StudioFrame::OnAddTrack)
	EVT_MENU(ID_EditTrackName, StudioFrame::OnEditTrackName)
	EVT_COMMAND(wxID_ANY, EVENT_SELECT_AUDIO, StudioFrame::OnSelectAudio)
	EVT_COMMAND(wxID_ANY, EVENT_ADD_AUDIO, StudioFrame::OnAddAudio)
	EVT_COMMAND(wxID_ANY, EVENT_REMOVE_AUDIO, StudioFrame::OnRemoveAudio)
END_EVENT_TABLE()

bool oamlStudio::OnInit() {
	oaml = new oamlApi();
	oaml->Init("oaml.defs");

	StudioFrame *frame = new StudioFrame(_("oamlStudio"), wxPoint(50, 50), wxSize(1024, 768));
	frame->Show(true);
	SetTopWindow(frame);
	return true;
}

StudioFrame::StudioFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame(NULL, -1, title, pos, size) {
	timer = NULL;

	wxMenuBar *menuBar = new wxMenuBar;
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_New, _("&New..."));
	menuFile->AppendSeparator();
	menuFile->Append(ID_Load, _("&Load..."));
	menuFile->Append(ID_Save, _("&Save..."));
	menuFile->Append(ID_SaveAs, _("&Save As..."));
	menuFile->Append(ID_Export, _("&Export..."));
	menuFile->AppendSeparator();
	menuFile->Append(ID_Quit, _("E&xit"));

	menuBar->Append(menuFile, _("&File"));

	menuFile = new wxMenu;
	menuFile->Append(ID_AddTrack, _("&Add track"));
	menuFile->AppendSeparator();

	menuBar->Append(menuFile, _("&Tracks"));

/*	menuFile = new wxMenu;
	menuFile->Append(ID_AddAudio, _("&Add audio"));
	menuFile->AppendSeparator();

	menuBar->Append(menuFile, _("&Audios"));*/

	menuFile = new wxMenu;
	menuFile->Append(ID_About, _("A&bout..."));
	menuFile->AppendSeparator();

	menuBar->Append(menuFile, _("&About"));

	SetMenuBar(menuBar);

	CreateStatusBar();
	SetStatusText(_("Ready"));

	mainSizer = new wxBoxSizer(wxHORIZONTAL);

	SetBackgroundColour(wxColour(0x40, 0x40, 0x40));

	tinfo = oaml->GetTracksInfo();

	trackList = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(240, -1), wxLC_LIST | wxLC_EDIT_LABELS | wxLC_SINGLE_SEL);
	trackList->SetBackgroundColour(wxColour(0x80, 0x80, 0x80));
	trackList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &StudioFrame::OnTrackListActivated, this);
	trackList->Bind(wxEVT_RIGHT_UP, &StudioFrame::OnTrackListMenu, this);
	trackList->Bind(wxEVT_LIST_END_LABEL_EDIT, &StudioFrame::OnTrackEndLabelEdit, this);
	for (size_t i=0; i<tinfo->tracks.size(); i++) {
		oamlTrackInfo *track = &tinfo->tracks[i];
		trackList->InsertItem(i, wxString(track->name));
	}

	trackPane = NULL;

	mainSizer->Add(trackList, 0, wxEXPAND | wxALL, 5);

	vSizer = new wxBoxSizer(wxVERTICAL);

	controlPane = new ControlPanel(this, wxID_ANY);
	vSizer->Add(controlPane, 0, wxEXPAND | wxALL, 5);

	mainSizer->Add(vSizer, 1, wxEXPAND | wxALL, 0);

	SetSizer(mainSizer);
	Layout();

	Centre(wxBOTH);
}

void StudioFrame::SelectTrack(std::string name) {
	if (trackPane) {
		trackPane->Destroy();
	}

	trackPane = new ScrolledWidgetsPane(this, wxID_ANY, name);
	vSizer->Add(trackPane, 1, wxEXPAND | wxALL, 5);

	oamlTrackInfo *track = GetTrackInfo(name);
	if (track == NULL)
		return;
	for (size_t i=0; i<track->audios.size(); i++) {
		trackPane->AddDisplay(track->audios[i].filename);
	}

	SetSizer(mainSizer);
	Layout();

	controlPane->SetTrack(name);
	controlPane->OnSelectAudio("");
}

void StudioFrame::OnTrackListActivated(wxListEvent& event) {
	int index = event.GetIndex();
	if (index == -1) {
//		WxUtils::ShowErrorDialog(_("You must choose a track!"));
		return;
	}

	wxString str = trackList->GetItemText(index);
	SelectTrack(str.ToStdString());
}

void StudioFrame::OnTrackListMenu(wxMouseEvent& WXUNUSED(event)) {
	wxMenu menu(wxT(""));
	menu.Append(ID_AddTrack, wxT("&Add Track"));
	menu.Append(ID_EditTrackName, wxT("Edit Track &Name"));
	PopupMenu(&menu);
}

void StudioFrame::OnTrackEndLabelEdit(wxCommandEvent& WXUNUSED(event)) {
	if (timer == NULL) {
		timer = new StudioTimer(this);
	}

	timer->StartOnce(10);
}

void StudioFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	Close(TRUE);
}

void StudioFrame::OnNew(wxCommandEvent& WXUNUSED(event)) {
	tinfo->tracks.clear();

	if (trackPane) {
		trackPane->Destroy();
	}
	trackList->ClearAll();
}

void StudioFrame::OnLoad(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog openFileDialog(this, _("Open oaml.defs"), ".", "oaml.defs", "*.defs", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	oaml->Init(openFileDialog.GetPath());

	tinfo = oaml->GetTracksInfo();

	for (size_t i=0; i<tinfo->tracks.size(); i++) {
		oamlTrackInfo *track = &tinfo->tracks[i];
		trackList->InsertItem(i, wxString(track->name));
	}
}

void StudioFrame::AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, const char *value) {
	tinyxml2::XMLElement *el = node->GetDocument()->NewElement(name);
	el->SetText(value);
	node->InsertEndChild(el);
}

void StudioFrame::AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, int value) {
	tinyxml2::XMLElement *el = node->GetDocument()->NewElement(name);
	el->SetText(value);
	node->InsertEndChild(el);
}

void StudioFrame::OnSave(wxCommandEvent& WXUNUSED(event)) {
	tinyxml2::XMLDocument xmlDoc;

	xmlDoc.InsertFirstChild(xmlDoc.NewDeclaration());

	for (size_t i=0; i<tinfo->tracks.size(); i++) {
		oamlTrackInfo *track = &tinfo->tracks[i];

		tinyxml2::XMLNode *trackEl = xmlDoc.NewElement("track");

		AddSimpleChildToNode(trackEl, "name", track->name.c_str());

		if (track->fadeIn) AddSimpleChildToNode(trackEl, "fadeIn", track->fadeIn);
		if (track->fadeOut) AddSimpleChildToNode(trackEl, "fadeOut", track->fadeOut);
		if (track->xfadeIn) AddSimpleChildToNode(trackEl, "xfadeIn", track->xfadeIn);
		if (track->xfadeOut) AddSimpleChildToNode(trackEl, "xfadeOut", track->xfadeOut);

		for (size_t j=0; j<track->audios.size(); j++) {
			oamlAudioInfo *audio = &track->audios[j];

			tinyxml2::XMLNode *audioEl = xmlDoc.NewElement("audio");
			AddSimpleChildToNode(audioEl, "filename", audio->filename.c_str());
			if (audio->type) AddSimpleChildToNode(audioEl, "type", audio->type);
			if (audio->bpm) AddSimpleChildToNode(audioEl, "bpm", audio->bpm);
			if (audio->beatsPerBar) AddSimpleChildToNode(audioEl, "beatsPerBar", audio->beatsPerBar);
			if (audio->bars) AddSimpleChildToNode(audioEl, "bars", audio->bars);
			if (audio->minMovementBars) AddSimpleChildToNode(audioEl, "minMovementBars", audio->minMovementBars);
			if (audio->randomChance) AddSimpleChildToNode(audioEl, "randomChance", audio->randomChance);
			if (audio->fadeIn) AddSimpleChildToNode(audioEl, "fadeIn", audio->fadeIn);
			if (audio->fadeOut) AddSimpleChildToNode(audioEl, "fadeOut", audio->fadeOut);
			if (audio->xfadeIn) AddSimpleChildToNode(audioEl, "xfadeIn", audio->xfadeIn);
			if (audio->xfadeOut) AddSimpleChildToNode(audioEl, "xfadeOut", audio->xfadeOut);

			trackEl->InsertEndChild(audioEl);
		}

		xmlDoc.InsertEndChild(trackEl);
	}

	xmlDoc.SaveFile(defsPath.c_str());
}

void StudioFrame::OnSaveAs(wxCommandEvent& event) {
	wxFileDialog openFileDialog(this, _("Save oaml.defs"), ".", "oaml.defs", "*.defs", wxFD_SAVE);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	defsPath = openFileDialog.GetPath();
	OnSave(event);
}

void StudioFrame::OnExport(wxCommandEvent& WXUNUSED(event)) {
}

void StudioFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	wxMessageBox(_("oamlStudio"), _("About oamlStudio"), wxOK | wxICON_INFORMATION, this);
}

void StudioFrame::OnAddTrack(wxCommandEvent& WXUNUSED(event)) {
	oamlTrackInfo track;
	memset(&track, 0, sizeof(oamlTrackInfo));

	char name[1024];
	snprintf(name, 1024, "Track%ld", tinfo->tracks.size()+1);
	track.name = name;

	tinfo->tracks.push_back(track);

	int index = tinfo->tracks.size()-1;
	trackList->InsertItem(index, wxString(track.name));
	SelectTrack(track.name);

	trackList->EditLabel(index);
}

void StudioFrame::OnEditTrackName(wxCommandEvent& WXUNUSED(event)) {
	trackList->EditLabel(trackList->GetFirstSelected());
}

void StudioFrame::OnSelectAudio(wxCommandEvent& event) {
	controlPane->OnSelectAudio(event.GetString().ToStdString());
}

void StudioFrame::OnAddAudio(wxCommandEvent& event) {
	trackPane->AddDisplay(event.GetString().ToStdString());

	SetSizer(mainSizer);
	Layout();
}

void StudioFrame::OnRemoveAudio(wxCommandEvent& event) {
	controlPane->OnSelectAudio("");

	trackPane->RemoveDisplay(event.GetString().ToStdString());

	SetSizer(mainSizer);
	Layout();
}
