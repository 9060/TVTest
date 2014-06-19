#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "GeneralOptions.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CGeneralOptions::CGeneralOptions()
	: m_DefaultDriverType(DEFAULT_DRIVER_LAST)
	, m_VideoRendererType(CVideoRenderer::RENDERER_DEFAULT)
	, m_CasDevice(-1)
	, m_fTemporaryNoDescramble(false)
	, m_fResident(false)
	, m_fKeepSingleTask(false)
	, m_DescrambleInstruction(0)
	, m_fDescrambleCurServiceOnly(false)
	, m_fEnableEmmProcess(false)
{
}


CGeneralOptions::~CGeneralOptions()
{
	Destroy();
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=App.CoreEngine;

	if ((Flags&UPDATE_CARDREADER)!=0) {
		App.Core.ChangeCasCard(m_CasDevice);
	}

	if ((Flags&UPDATE_RESIDENT)!=0) {
		App.UICore.SetResident(m_fResident);
	}

	if ((Flags&UPDATE_DESCRAMBLECURONLY)!=0) {
		if (!App.RecordManager.IsRecording())
			CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	}

	if ((Flags&UPDATE_ENABLEEMMPROCESS)!=0) {
		CoreEngine.m_DtvEngine.m_CasProcessor.EnableContract(m_fEnableEmmProcess);
	}

	return true;
}


bool CGeneralOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	TCHAR szDirectory[MAX_PATH];
	if (Settings.Read(TEXT("DriverDirectory"),szDirectory,lengthof(szDirectory))
			&& szDirectory[0]!='\0') {
		m_BonDriverDirectory=szDirectory;
		GetAppClass().CoreEngine.SetDriverDirectory(szDirectory);
	}

	if (Settings.Read(TEXT("DefaultDriverType"),&Value)
			&& Value>=DEFAULT_DRIVER_NONE && Value<=DEFAULT_DRIVER_CUSTOM)
		m_DefaultDriverType=(DefaultDriverType)Value;
	Settings.Read(TEXT("DefaultDriver"),&m_DefaultBonDriverName);
	Settings.Read(TEXT("Driver"),&m_LastBonDriverName);

	Settings.Read(TEXT("Mpeg2Decoder"),&m_Mpeg2DecoderName);
	Settings.Read(TEXT("H264Decoder"),&m_H264DecoderName);
	Settings.Read(TEXT("H265Decoder"),&m_H265DecoderName);

	TCHAR szRenderer[16];
	if (Settings.Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
		if (szRenderer[0]=='\0') {
			m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		} else {
			CVideoRenderer::RendererType Renderer=CVideoRenderer::ParseName(szRenderer);
			if (Renderer!=CVideoRenderer::RENDERER_UNDEFINED)
				m_VideoRendererType=Renderer;
		}
	}

	if (!Settings.Read(TEXT("CasDevice"),&m_CasDeviceName)) {
		// �ȑO�̃o�[�W�����Ƃ̌݊��p
		if (Settings.Read(TEXT("CardReader"),&Value)) {
			if (Value>=0)
				m_CasDevice=Value-1;
		}
	}

	if (Settings.Read(TEXT("DescrambleInstruction"),&Value)) {
		m_DescrambleInstruction=Value;
	} else {
		// �ȑO�̃o�[�W�����Ƃ̌݊��p
		bool f;
		if (Settings.Read(TEXT("DescrambleSSSE3"),&f) && f) {
			m_DescrambleInstruction=2;
		} else if (Settings.Read(TEXT("DescrambleSSE2"),&f)) {
			if (f)
				m_DescrambleInstruction=1;
			else
				m_DescrambleInstruction=0;
		}
	}

	Settings.Read(TEXT("DescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	Settings.Read(TEXT("ProcessEMM"),&m_fEnableEmmProcess);

	Settings.Read(TEXT("Resident"),&m_fResident);
	Settings.Read(TEXT("KeepSingleTask"),&m_fKeepSingleTask);
	return true;
}


bool CGeneralOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DriverDirectory"),m_BonDriverDirectory);
	Settings.Write(TEXT("DefaultDriverType"),(int)m_DefaultDriverType);
	Settings.Write(TEXT("DefaultDriver"),m_DefaultBonDriverName);
	Settings.Write(TEXT("Driver"),GetAppClass().CoreEngine.GetDriverFileName());
	Settings.Write(TEXT("Mpeg2Decoder"),m_Mpeg2DecoderName);
	Settings.Write(TEXT("H264Decoder"),m_H264DecoderName);
	Settings.Write(TEXT("H265Decoder"),m_H265DecoderName);
	Settings.Write(TEXT("Renderer"),
				   CVideoRenderer::EnumRendererName((int)m_VideoRendererType));
	Settings.Write(TEXT("CasDevice"),m_CasDeviceName);
	/*
	Settings.Write(TEXT("DescrambleSSE2"),m_DescrambleInstruction!=CTsDescrambler::INSTRUCTION_NORMAL);
	Settings.Write(TEXT("DescrambleSSSE3"),m_DescrambleInstruction==CTsDescrambler::INSTRUCTION_SSSE3);
	*/
	Settings.Write(TEXT("DescrambleInstruction"),m_DescrambleInstruction);
	Settings.Write(TEXT("DescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	Settings.Write(TEXT("ProcessEMM"),m_fEnableEmmProcess);
	Settings.Write(TEXT("Resident"),m_fResident);
	Settings.Write(TEXT("KeepSingleTask"),m_fKeepSingleTask);
	return true;
}


bool CGeneralOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_GENERAL));
}


CGeneralOptions::DefaultDriverType CGeneralOptions::GetDefaultDriverType() const
{
	return m_DefaultDriverType;
}


LPCTSTR CGeneralOptions::GetDefaultDriverName() const
{
	return m_DefaultBonDriverName.c_str();
}


bool CGeneralOptions::SetDefaultDriverName(LPCTSTR pszDriverName)
{
	if (pszDriverName==NULL)
		m_DefaultBonDriverName.clear();
	else
		m_DefaultBonDriverName=pszDriverName;
	return true;
}


bool CGeneralOptions::GetFirstDriverName(LPTSTR pszDriverName) const
{
	switch (m_DefaultDriverType) {
	case DEFAULT_DRIVER_NONE:
		pszDriverName[0]='\0';
		break;
	case DEFAULT_DRIVER_LAST:
		if (m_LastBonDriverName.length()>=MAX_PATH)
			return false;
		::lstrcpy(pszDriverName,m_LastBonDriverName.c_str());
		break;
	case DEFAULT_DRIVER_CUSTOM:
		if (m_DefaultBonDriverName.length()>=MAX_PATH)
			return false;
		::lstrcpy(pszDriverName,m_DefaultBonDriverName.c_str());
		break;
	default:
		return false;
	}
	return true;
}


LPCTSTR CGeneralOptions::GetMpeg2DecoderName() const
{
	return m_Mpeg2DecoderName.c_str();
}


bool CGeneralOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_Mpeg2DecoderName.clear();
	else
		m_Mpeg2DecoderName=pszDecoderName;
	return true;
}


LPCTSTR CGeneralOptions::GetH264DecoderName() const
{
	return m_H264DecoderName.c_str();
}


bool CGeneralOptions::SetH264DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_H264DecoderName.clear();
	else
		m_H264DecoderName=pszDecoderName;
	return true;
}


LPCTSTR CGeneralOptions::GetH265DecoderName() const
{
	return m_H265DecoderName.c_str();
}


bool CGeneralOptions::SetH265DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_H265DecoderName.clear();
	else
		m_H265DecoderName=pszDecoderName;
	return true;
}


CVideoRenderer::RendererType CGeneralOptions::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CGeneralOptions::SetVideoRendererType(CVideoRenderer::RendererType Renderer)
{
	if (CVideoRenderer::EnumRendererName((int)Renderer)==NULL)
		return false;
	m_VideoRendererType=Renderer;
	return true;
}


int CGeneralOptions::GetCasDevice(bool fUseName)
{
	if (fUseName) {
		if (!m_CasDeviceName.empty()) {
			m_CasDevice=GetAppClass().CoreEngine.m_DtvEngine.m_CasProcessor.GetCasDeviceByName(m_CasDeviceName.c_str());
		} else {
			m_CasDevice=-1;
		}
	}

	return m_CasDevice;
}


bool CGeneralOptions::SetCasDevice(int Device)
{
	if (Device>=0) {
		CCasProcessor::CasDeviceInfo DeviceInfo;

		if (!GetAppClass().CoreEngine.m_DtvEngine.m_CasProcessor.GetCasDeviceInfo(Device,&DeviceInfo))
			return false;
		m_CasDeviceName=DeviceInfo.Name;
	} else {
		if (Device<-1)
			return false;
		m_CasDeviceName.clear();
	}

	m_CasDevice=Device;

	return true;
}


void CGeneralOptions::SetTemporaryNoDescramble(bool fNoDescramble)
{
	m_fTemporaryNoDescramble=fNoDescramble;
}


bool CGeneralOptions::GetResident() const
{
	return m_fResident;
}


bool CGeneralOptions::GetKeepSingleTask() const
{
	return m_fKeepSingleTask;
}


bool CGeneralOptions::GetDescrambleCurServiceOnly() const
{
	return m_fDescrambleCurServiceOnly;
}


bool CGeneralOptions::GetEnableEmmProcess() const
{
	return m_fEnableEmmProcess;
}


INT_PTR CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &App=GetAppClass();

			::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,m_BonDriverDirectory.c_str());

			// BonDriver
			::CheckRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
									IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
							   (int)m_DefaultDriverType+IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
						   m_DefaultDriverType==DEFAULT_DRIVER_CUSTOM);

			const CDriverManager &DriverManager=App.DriverManager;
			DlgComboBox_LimitText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,MAX_PATH-1);
			for (int i=0;i<DriverManager.NumDrivers();i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									  DriverManager.GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,m_DefaultBonDriverName.c_str());

			// MPEG-2 �f�R�[�_
			SetVideoDecoderList(IDC_OPTIONS_MPEG2DECODER,
								MEDIASUBTYPE_MPEG2_VIDEO,
								STREAM_TYPE_MPEG2_VIDEO,
								m_Mpeg2DecoderName);
			// H.264 �f�R�[�_
			SetVideoDecoderList(IDC_OPTIONS_H264DECODER,
								MEDIASUBTYPE_H264,
								STREAM_TYPE_H264,
								m_H264DecoderName);
			// H.265 �f�R�[�_
			SetVideoDecoderList(IDC_OPTIONS_H265DECODER,
								MEDIASUBTYPE_HEVC,
								STREAM_TYPE_H265,
								m_H265DecoderName);

			// �f�������_��
			LPCTSTR pszRenderer;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,TEXT("�f�t�H���g"));
			for (int i=1;(pszRenderer=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,pszRenderer);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RENDERER,m_VideoRendererType);

			// �J�[�h���[�_�[
			{
				CCoreEngine &CoreEngine=App.CoreEngine;
				CCasProcessor &CasProcessor=CoreEngine.m_DtvEngine.m_CasProcessor;
				if (!CasProcessor.IsCasLibraryLoaded())
					App.Core.LoadCasLibrary(NULL);
				m_fEnableCasSettings=CasProcessor.IsCasLibraryLoaded();

				CCoreEngine::CasDeviceList CasDevList;
				int Sel=0;
				CoreEngine.GetCasDeviceList(&CasDevList);
				for (size_t i=0;i<CasDevList.size();i++) {
					DlgComboBox_AddString(hDlg,IDC_OPTIONS_CARDREADER,
										  CasDevList[i].Text.c_str());
					DlgComboBox_SetItemData(hDlg,IDC_OPTIONS_CARDREADER,
											i,CasDevList[i].Device);
					if (CasDevList[i].Device==m_CasDevice)
						Sel=(int)i;
				}
				DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_CARDREADER,
									  m_fTemporaryNoDescramble?0:Sel);
				if (!m_fEnableCasSettings)
					EnableDlgItem(hDlg,IDC_OPTIONS_CARDREADER,false);

				DlgCheckBox_Check(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY,m_fDescrambleCurServiceOnly);
				DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS,m_fEnableEmmProcess);
				if (!m_fEnableCasSettings || m_CasDevice<0) {
					EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLE_FIRST,
										IDC_OPTIONS_DESCRAMBLE_LAST,false);
				}

				if (m_fEnableCasSettings) {
					const UINT AvailableInstructions=CasProcessor.GetAvailableInstructions();
					TCHAR szName[64];
					for (int i=0;CasProcessor.GetInstructionName(i,szName,lengthof(szName))>0;i++) {
						if ((AvailableInstructions&(1U<<i))!=0) {
							LRESULT Index=DlgComboBox_AddString(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,szName);
							DlgComboBox_SetItemData(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,Index,i);
						}
					}
					DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,m_DescrambleInstruction);
				} else {
					EnableDlgItems(hDlg,
								   IDC_OPTIONS_DESCRAMBLEINSTRUCTION_LABEL,
								   IDC_OPTIONS_DESCRAMBLEBENCHMARK,
								   false);
				}
			}

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESIDENT,m_fResident);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_KEEPSINGLETASK,m_fKeepSingleTask);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVERDIRECTORY_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH];

				if (::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory,lengthof(szDirectory))>0) {
					if (::PathIsRelative(szDirectory)) {
						TCHAR szTemp[MAX_PATH];

						GetAppClass().GetAppDirectory(szTemp);
						::PathAppend(szTemp,szDirectory);
						::PathCanonicalize(szDirectory,szTemp);
					}
				} else {
					GetAppClass().GetAppDirectory(szDirectory);
				}
				if (BrowseFolderDialog(hDlg,szDirectory,TEXT("BonDriver �̌����t�H���_��I�����Ă��������B")))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory);
			}
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_NONE:
		case IDC_OPTIONS_DEFAULTDRIVER_LAST:
		case IDC_OPTIONS_DEFAULTDRIVER_CUSTOM:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									   IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
									   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM);
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("���ׂẴt�@�C��\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriver�̑I��");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName);
			}
			return TRUE;

		case IDC_OPTIONS_CARDREADER:
			if (HIWORD(wParam)==CBN_SELCHANGE
					&& m_fEnableCasSettings) {
				EnableDlgItems(hDlg,
					IDC_OPTIONS_DESCRAMBLE_FIRST,
					IDC_OPTIONS_DESCRAMBLE_LAST,
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER)>0);
			}
			return TRUE;

		case IDC_OPTIONS_DESCRAMBLEBENCHMARK:
			if (::MessageBox(hDlg,
					TEXT("�x���`�}�[�N�e�X�g���J�n���܂��B\n")
					TEXT("�I������܂ő���͍s��Ȃ��悤�ɂ��Ă��������B\n")
					TEXT("���ʂ͂΂��������܂��̂ŁA������s���Ă��������B"),
					TEXT("�x���`�}�[�N�e�X�g"),
					MB_OKCANCEL | MB_ICONINFORMATION)==IDOK) {
				DescrambleBenchmarkTest(hDlg);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RENDERER);
				if (Renderer!=m_VideoRendererType) {
					if (!CVideoRenderer::IsAvailable(Renderer)) {
						SettingError();
						::MessageBox(hDlg,TEXT("�I�����ꂽ�����_���͂��̊��ŗ��p�\�ɂȂ��Ă��܂���B"),
									 NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					m_VideoRendererType=Renderer;
					SetUpdateFlag(UPDATE_RENDERER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				TCHAR szDirectory[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,
								 szDirectory,lengthof(szDirectory));
				m_BonDriverDirectory=szDirectory;

				m_DefaultDriverType=(DefaultDriverType)
					(GetCheckedRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
										   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM)-
					IDC_OPTIONS_DEFAULTDRIVER_NONE);

				TCHAR szDefaultBonDriver[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								 szDefaultBonDriver,lengthof(szDefaultBonDriver));
				m_DefaultBonDriverName=szDefaultBonDriver;

				GetVideoDecoderSetting(IDC_OPTIONS_MPEG2DECODER,
									   STREAM_TYPE_MPEG2_VIDEO,
									   &m_Mpeg2DecoderName);
				GetVideoDecoderSetting(IDC_OPTIONS_H264DECODER,
									   STREAM_TYPE_H264,
									   &m_H264DecoderName);
				GetVideoDecoderSetting(IDC_OPTIONS_H265DECODER,
									   STREAM_TYPE_H265,
									   &m_H265DecoderName);

				if (m_fEnableCasSettings) {
					int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER);
					int CasDevice=(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_CARDREADER,Sel);
					if ((m_fTemporaryNoDescramble && Sel>0)
							|| (!m_fTemporaryNoDescramble && CasDevice!=m_CasDevice)) {
						SetCasDevice(CasDevice);
						m_fTemporaryNoDescramble=false;
						SetUpdateFlag(UPDATE_CARDREADER);
					}

					Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION);
					if (Sel>=0) {
						m_DescrambleInstruction=
							(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,Sel);
					}

					bool fCurOnly=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY);
					if (fCurOnly!=m_fDescrambleCurServiceOnly) {
						m_fDescrambleCurServiceOnly=fCurOnly;
						SetUpdateFlag(UPDATE_DESCRAMBLECURONLY);
					}

					bool fEmm=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS);
					if (fEmm!=m_fEnableEmmProcess) {
						m_fEnableEmmProcess=fEmm;
						SetUpdateFlag(UPDATE_ENABLEEMMPROCESS);
					}
				}

				bool fResident=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESIDENT);
				if (fResident!=m_fResident) {
					m_fResident=fResident;
					SetUpdateFlag(UPDATE_RESIDENT);
				}

				m_fKeepSingleTask=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_KEEPSINGLETASK);

				m_fChanged=true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CGeneralOptions::SetVideoDecoderList(
	int ID,const GUID &SubType,BYTE StreamType,const TVTest::String &DecoderName)
{
	CDirectShowFilterFinder FilterFinder;
	int Count=0;
	if (FilterFinder.FindFilter(&MEDIATYPE_Video,&SubType)) {
		for (int i=0;i<FilterFinder.GetFilterCount();i++) {
			WCHAR szFilterName[MAX_VIDEO_DECODER_NAME];

			if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
				DlgComboBox_AddString(m_hDlg,ID,szFilterName);
				Count++;
			}
		}
	}
	int Sel=0;
	if (Count==0) {
		DlgComboBox_AddString(m_hDlg,ID,TEXT("<�f�R�[�_�����t����܂���>"));
	} else {
		CMediaViewer &MediaViewer=GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
		TCHAR szText[32+MAX_VIDEO_DECODER_NAME];

		::lstrcpy(szText,TEXT("����"));
		if (!DecoderName.empty()) {
			Sel=(int)DlgComboBox_FindStringExact(m_hDlg,ID,-1,DecoderName.c_str())+1;
		} else if (MediaViewer.IsOpen()
				&& StreamType==MediaViewer.GetVideoStreamType()) {
			::lstrcat(szText,TEXT(" ("));
			MediaViewer.GetVideoDecoderName(szText+::lstrlen(szText),MAX_VIDEO_DECODER_NAME);
			::lstrcat(szText,TEXT(")"));
		}
		DlgComboBox_InsertString(m_hDlg,ID,0,szText);
	}
	DlgComboBox_SetCurSel(m_hDlg,ID,Sel);
}


void CGeneralOptions::GetVideoDecoderSetting(
	int ID,BYTE StreamType,TVTest::String *pDecoderName)
{
	TCHAR szDecoder[MAX_VIDEO_DECODER_NAME];
	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,ID);
	if (Sel>0)
		DlgComboBox_GetLBString(m_hDlg,ID,Sel,szDecoder);
	else
		szDecoder[0]='\0';
	if (::lstrcmpi(szDecoder,pDecoderName->c_str())!=0) {
		*pDecoderName=szDecoder;
		SetUpdateFlag(UPDATE_DECODER);
		if (StreamType==GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoStreamType())
			SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
	}
}


// �x���`�}�[�N�e�X�g
void CGeneralOptions::DescrambleBenchmarkTest(HWND hwndOwner)
{
	CCasProcessor &CasProcessor=GetAppClass().CoreEngine.m_DtvEngine.m_CasProcessor;
	const UINT AvailableInstructions=CasProcessor.GetAvailableInstructions();
	if (AvailableInstructions==0)
		return;

	HCURSOR hcurOld=::SetCursor(LoadCursor(NULL,IDC_WAIT));

	static const DWORD BENCHMARK_ROUND=200000;
	DWORD BenchmarkCount=0,MaxTime=0;
	DWORD Times[32];
	::ZeroMemory(Times,sizeof(Times));

	do {
		for (int i=0;AvailableInstructions>>i!=0;i++) {
			if (((AvailableInstructions>>i)&1)!=0) {
				DWORD Time;
				CasProcessor.DescrambleBenchmarkTest(i,BENCHMARK_ROUND,&Time);
				Times[i]+=Time;
				if (Times[i]>MaxTime)
					MaxTime=Times[i];
			}
		}
		BenchmarkCount+=BENCHMARK_ROUND;
	} while (MaxTime<1500);

	::SetCursor(hcurOld);

	TCHAR szCPU[256];
	DWORD Type=REG_SZ,Size=sizeof(szCPU);
	if (::SHGetValue(HKEY_LOCAL_MACHINE,
					 TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
					 TEXT("ProcessorNameString"),
					 &Type,szCPU,&Size)!=ERROR_SUCCESS
			|| Type!=REG_SZ)
		szCPU[0]='\0';

	TCHAR szText[1024];
	CStaticStringFormatter Formatter(szText,lengthof(szText));
	if (szCPU[0]!='\0')
		Formatter.AppendFormat(TEXT("%s\n"),szCPU);
	Formatter.AppendFormat(TEXT("%u ��̎��s�Ɋ|����������\n"),BenchmarkCount);
	DWORD NormalTime=Times[0];
	DWORD MinTime=0xFFFFFFFF;
	int Fastest=0;
	for (int i=0;AvailableInstructions>>i!=0;i++) {
		if (((AvailableInstructions>>i)&1)!=0) {
			const DWORD Time=Times[i];
			TCHAR szName[64];

			CasProcessor.GetInstructionName(i,szName,lengthof(szName));
			Formatter.AppendFormat(TEXT("%s : %u ms (%d �p�P�b�g/�b)"),
				szName,Time,::MulDiv(BenchmarkCount,1000,Time));
			if (i>0 && NormalTime>0 && Time>0) {
				int Percentage;
				if (NormalTime>=Time)
					Percentage=(int)(NormalTime*100/Time)-100;
				else
					Percentage=-(int)((Time*100/NormalTime)-100);
				Formatter.AppendFormat(TEXT(" (����������銄�� %d %%)"),Percentage);
			}
			Formatter.Append(TEXT("\n"));

			if (Time<MinTime) {
				MinTime=Time;
				Fastest=i;
			}
		}
	}

	TCHAR szName[64];
	CasProcessor.GetInstructionName(Fastest,szName,lengthof(szName));
	Formatter.AppendFormat(TEXT("\n%s �ɂ��邱�Ƃ������߂��܂��B"),szName);

	CMessageDialog MessageDialog;
	MessageDialog.Show(hwndOwner,CMessageDialog::TYPE_INFO,Formatter.GetString(),
					   TEXT("�x���`�}�[�N�e�X�g����"),NULL,TEXT("�x���`�}�[�N�e�X�g"));
}
