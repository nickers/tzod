// gui.cpp

#include "stdafx.h"

#include "gui.h"
#include "gui_settings.h"
#include "gui_desktop.h"

#include "GuiManager.h"

#include "Button.h"
#include "List.h"
#include "Text.h"
#include "Edit.h"
#include "Combo.h"


#include "functions.h"
#include "level.h"
#include "macros.h"

#include "video/TextureManager.h"

#include "config/Config.h"

#include "core/Console.h"

#include "fs/FileSystem.h"
#include "fs/MapFile.h"
#include "gc/Player.h"
#include "gc/ai.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MainMenuDlg::MainMenuDlg(Window *parent) : Dialog(parent, 0, 0, 1, 1, false)
{
	SetBorder(false);
	SetTexture("gui_splash");
	Resize(GetTextureWidth(), GetTextureHeight());

	OnParentSize(parent->GetWidth(), parent->GetHeight());

	(new Button(this, 0, GetHeight(), "���� (F2)"))
		->eventClick.bind(&MainMenuDlg::OnNewGame, this);

//	new Button(this, 0, GetHeight() + 30, "���������");
//	new Button(this, 0, GetHeight() + 60, "���������");

//	new Button(this, 100, GetHeight(), "Host");
//	new Button(this, 100, GetHeight() + 30, "Join");
//	new Button(this, 100, GetHeight() + 60, "�������");

//	new Button(this, 200, GetHeight(), "New map");
//	new Button(this, 200, GetHeight() + 30, "Import");
//	new Button(this, 200, GetHeight() + 60, "Export");

	(new Button(this, 300, GetHeight(), "����� (F12)"))
		->eventClick.bind(&MainMenuDlg::OnSettings, this);

	(new Button(this, 416, GetHeight(), "����� (Alt+�4)"))
		->eventClick.bind(&MainMenuDlg::OnExit, this);


	std::set<string_t> c;
	g_fs->GetFileSystem("campaign")->EnumAllFiles(c, "*.lua");

	float y = GetHeight() + 30;
	for( std::set<string_t>::iterator it = c.begin(); it != c.end(); ++it )
	{
		it->erase(it->length() - 4); // cut out the file extension

		DelegateAdapter1<string_t> d(*it);
		d.eventOnEvent.bind(&MainMenuDlg::OnCampaign, this);
		_campaigns.push_back(d);

		Button *btn = new Button(this, 0, y, it->c_str());
		btn->eventClick.bind(&DelegateAdapter1<string_t>::OnEvent, &_campaigns.back());

		y += btn->GetHeight() + 1;
	}
}

void MainMenuDlg::OnCampaign(string_t name)
{
	Close(_resultOK);
	if( !script_exec_file(g_env.L, ("campaign/" + name + ".lua").c_str()) )
	{
		static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
	}
}

void MainMenuDlg::OnNewGame()
{
	NewGameDlg *dlg = new NewGameDlg(this);
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnExit()
{
	DestroyWindow(g_env.hMainWnd);
}

void MainMenuDlg::OnSettings()
{
	SettingsDlg *dlg = new SettingsDlg(this);
//	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);	
}

void MainMenuDlg::OnParentSize(float width, float height)
{
	Move( (width - GetWidth()) * 0.5f, (height - GetHeight()) * 0.5f );
}

void MainMenuDlg::OnCloseChild(int result)
{
	if( Dialog::_resultOK == result )
	{
		Close(result);
	}
}

void MainMenuDlg::OnRawChar(int c)
{
	switch(c)
	{
	case VK_F2:
		OnNewGame();
		break;
	case VK_F12:
		OnSettings();
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////

NewGameDlg::NewGameDlg(Window *parent) : Dialog(parent, 0, 0, 770, 550, true)
{
	Move( (parent->GetWidth() - GetWidth()) / 2, (parent->GetHeight() - GetHeight()) / 2 );
	_newPlayer = false;


	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 16;


	//
	// map list
	//

	new Text(this, 16, 16, "�������� �����", alignTextLT);

	_maps = new List(this, x1, 32, x2 - x1, 192);
	_maps->SetTabPos(0,   4); // name
	_maps->SetTabPos(1, 384); // size
	_maps->SetTabPos(2, 448); // theme
	GetManager()->SetFocusWnd(_maps);


	SafePtr<IFileSystem> dir = g_fs->GetFileSystem(DIR_MAPS);
	if( dir )
	{
		std::set<string_t> files;
		if( dir->EnumAllFiles(files, TEXT("*.map")) )
		{
			int lastMapIndex = 0;

			for( std::set<string_t>::iterator it = files.begin(); it != files.end(); ++it )
			{
				string_t tmp = DIR_MAPS;
				tmp += "/";
				tmp += *it;

				MapFile file;
				if( file.Open(tmp.c_str(), false) )
				{
					it->erase(it->length() - 4); // cut out the file extension
					int index = _maps->AddItem(it->c_str());

					if( *it == g_conf.cl_map->Get() )
						lastMapIndex = index;

					char size[64];
					int h = 0, w = 0;
					file.getMapAttribute("width", w);
					file.getMapAttribute("height", h);
					wsprintf(size, "%3d*%d", w, h);
					_maps->SetItemText(index, 1, size);

					if( file.getMapAttribute("theme", tmp) )
					{
						_maps->SetItemText(index, 2, tmp.c_str());
					}
				}
			}

			_maps->SetCurSel(lastMapIndex, false);
			_maps->ScrollTo(lastMapIndex - (_maps->GetNumLinesVisible() - 1) * 0.5f);
		}
		else
		{
			_ASSERT(FALSE); // EnumAllFiles has returned error...
		}
	}


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = new CheckBox(this, x3, y, "������ �����");
		_nightMode->SetCheck( g_conf.cl_nightmode->Get() );


		new Text(this, x3, y+=30, "�������� ����, %", alignTextLT);
		_gameSpeed = new Edit(this, x3+20, y+=15, 80);
		_gameSpeed->SetInt(g_conf.cl_speed->GetInt());

		new Text(this, x3, y+=30, "����� ������", alignTextLT);
		_fragLimit = new Edit(this, x3+20, y+=15, 80);
		_fragLimit->SetInt(g_conf.cl_fraglimit->GetInt());

		new Text(this, x3, y+=30, "����� �������", alignTextLT);
		_timeLimit = new Edit(this, x3+20, y+=15, 80);
		_timeLimit->SetInt(g_conf.cl_timelimit->GetInt());

		new Text(this, x3+30, y+=40, "(0 - ��� ������)", alignTextLT);
	}



	//
	// player list
	//

	new Text(this, 16, 240, "������", alignTextLT);

	_players = new List(this, x1, 256, x2-x1, 96);
	_players->SetTabPos(0,   4); // name
	_players->SetTabPos(1, 192); // skin
	_players->SetTabPos(2, 256); // class
	_players->SetTabPos(3, 320); // team
	_players->eventChangeCurSel.bind(&NewGameDlg::OnSelectPlayer, this);


	new Text(this, 16, 368, "����", alignTextLT);
	_bots = new List(this, x1, 384, x2-x1, 96);
	_bots->SetTabPos(0,   4); // name
	_bots->SetTabPos(1, 192); // skin
	_bots->SetTabPos(2, 256); // class
	_bots->SetTabPos(3, 320); // team
	_bots->eventChangeCurSel.bind(&NewGameDlg::OnSelectBot, this);


	//
	// buttons
	//

	{
		Button *btn;


		btn = new Button(this, x3, 256, "����� (Ins)");
		btn->eventClick.bind(&NewGameDlg::OnAddPlayer, this);

		_removePlayer = new Button(this, x3, 286, "�������");
		_removePlayer->eventClick.bind(&NewGameDlg::OnRemovePlayer, this);
		_removePlayer->Enable(false);

		_changePlayer = new Button(this, x3, 316, "��������");
		_changePlayer->eventClick.bind(&NewGameDlg::OnEditPlayer, this);
		_changePlayer->Enable(false);

		btn = new Button(this, x3, 384, "�����");
		btn->eventClick.bind(&NewGameDlg::OnAddBot, this);

		_removeBot = new Button(this, x3, 414, "�������");
		_removeBot->eventClick.bind(&NewGameDlg::OnRemoveBot, this);
		_removeBot->Enable(false);

		_changeBot = new Button(this, x3, 444, "��������");
		_changeBot->eventClick.bind(&NewGameDlg::OnEditBot, this);
		_changeBot->Enable(false);



		btn = new Button(this, 544, 510, "�������!");
		btn->eventClick.bind(&NewGameDlg::OnOK, this);

		btn = new Button(this, 656, 510, "������");
		btn->eventClick.bind(&NewGameDlg::OnCancel, this);
	}

	// call this after creation of buttons
	RefreshPlayersList();
	RefreshBotsList();
}

void NewGameDlg::RefreshPlayersList()
{
	int selected = _players->GetCurSel();
	_players->DeleteAllItems();

	for( size_t i = 0; i < g_conf.dm_players->GetSize(); ++i )
	{
		ConfVarTable *p = g_conf.dm_players->GetAt(i)->AsTable();

		int index = _players->AddItem( p->GetStr("nick")->Get() );
		_players->SetItemText(index, 1, p->GetStr("skin")->Get());
		_players->SetItemText(index, 2, p->GetStr("class")->Get());

		char s[16];
		int team = p->GetNum("team", 0)->GetInt();
		if( 0 != team )
		{
			wsprintf(s, "%d", team);
		}
		else 
		{
			wsprintf(s, "[���]");
		}

		_players->SetItemText(index, 3, s);
	}

	_players->SetCurSel(__min(selected, _players->GetSize()-1));
}

void NewGameDlg::RefreshBotsList()
{
	int selected = _bots->GetCurSel();
	_bots->DeleteAllItems();

	for( size_t i = 0; i < g_conf.dm_bots->GetSize(); ++i )
	{
		ConfVarTable *p = g_conf.dm_bots->GetAt(i)->AsTable();

		int index = _bots->AddItem( p->GetStr("nick")->Get() );
		_bots->SetItemText(index, 1, p->GetStr("skin")->Get());
		_bots->SetItemText(index, 2, p->GetStr("class")->Get());

		char s[16];
		int team = p->GetNum("team", 0)->GetInt();
		if( 0 != team )
		{
			wsprintf(s, "%d", team);
		}
		else 
		{
			wsprintf(s, "[���]");
		}

		_bots->SetItemText(index, 3, s);
	}

	_bots->SetCurSel(__min(selected, _bots->GetSize()-1));
}

void NewGameDlg::OnAddPlayer()
{
	std::vector<string_t> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/", true);

	ConfVarTable *p = g_conf.dm_players->PushBack(ConfVar::typeTable)->AsTable();
	p->SetStr("skin", skinNames[rand() % skinNames.size()].c_str());

	_newPlayer = true;
	(new EditPlayerDlg(this, p))->eventClose.bind( &NewGameDlg::OnAddPlayerClose, this );
}

void NewGameDlg::OnAddPlayerClose(int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
	else if( _newPlayer )
	{
		g_conf.dm_players->PopBack();
	}
	_newPlayer = false;
}

void NewGameDlg::OnRemovePlayer()
{
	_ASSERT( -1 != _players->GetCurSel() );
	g_conf.dm_players->RemoveAt(_players->GetCurSel());
	RefreshPlayersList();
}

void NewGameDlg::OnEditPlayer()
{
	int index = _players->GetCurSel();
	_ASSERT(-1 != index);

	(new EditPlayerDlg(this, g_conf.dm_players->GetAt(index)->AsTable()))
		->eventClose.bind( &NewGameDlg::OnEditPlayerClose, this );
}

void NewGameDlg::OnEditPlayerClose(int result)
{
	if( _resultOK == result )
	{
		RefreshPlayersList();
	}
}

void NewGameDlg::OnAddBot()
{
	std::vector<string_t> skinNames;
	g_texman->GetTextureNames(skinNames, "skin/", true);

	ConfVarTable *p = g_conf.dm_bots->PushBack(ConfVar::typeTable)->AsTable();
	p->SetStr("skin", skinNames[rand() % skinNames.size()].c_str());

	_newPlayer = true;
	(new EditBotDlg(this, p))->eventClose.bind( &NewGameDlg::OnAddBotClose, this );
}

void NewGameDlg::OnAddBotClose(int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
	else if( _newPlayer )
	{
		g_conf.dm_bots->PopBack();
	}
	_newPlayer = false;
}

void NewGameDlg::OnRemoveBot()
{
	_ASSERT( -1 != _bots->GetCurSel() );
	g_conf.dm_bots->RemoveAt(_bots->GetCurSel());
	RefreshBotsList();
}

void NewGameDlg::OnEditBot()
{
	int index = _bots->GetCurSel();
	_ASSERT(-1 != index);

	(new EditBotDlg(this, g_conf.dm_bots->GetAt(index)->AsTable()))
		->eventClose.bind( &NewGameDlg::OnEditBotClose, this );
}

void NewGameDlg::OnEditBotClose(int result)
{
	if( _resultOK == result )
	{
		RefreshBotsList();
	}
}

void NewGameDlg::OnOK()
{
	string_t fn;
	int index = _maps->GetCurSel();
	if( -1 != index )
	{
		fn = _maps->GetItemText(index, 0);
	}
	else
	{
//		MessageBoxT(NULL, "�������� �����", MB_OK|MB_ICONHAND);
		return;
	}

	string_t path = DIR_MAPS;
	path += "\\";
	path += fn + ".map";

	g_conf.cl_speed->SetInt( __max(MIN_GAMESPEED, __min(MAX_GAMESPEED, _gameSpeed->GetInt())) );
	g_conf.cl_fraglimit->SetInt( __max(0, __min(MAX_FRAGLIMIT, _fragLimit->GetInt())) );
	g_conf.cl_timelimit->SetInt( __max(0, __min(MAX_TIMELIMIT, _timeLimit->GetInt())) );
	g_conf.cl_nightmode->Set( _nightMode->GetCheck() );

	g_conf.sv_speed->SetInt( g_conf.cl_speed->GetInt() );
	g_conf.sv_fraglimit->SetInt( g_conf.cl_fraglimit->GetInt() );
	g_conf.sv_timelimit->SetInt( g_conf.cl_timelimit->GetInt() );
	g_conf.sv_nightmode->Set( g_conf.cl_nightmode->Get() );

	script_exec(g_env.L, "reset()");
	_ASSERT(!g_level);
	g_level = new Level();

	if( g_level->init_newdm(path.c_str()) )
	{
		g_conf.cl_map->Set(fn.c_str());

		for( size_t i = 0; i < g_conf.dm_players->GetSize(); ++i )
		{
			ConfVarTable *p = g_conf.dm_players->GetAt(i)->AsTable();
			GC_PlayerLocal *player = new GC_PlayerLocal();
			player->SetTeam(    p->GetNum("team")->GetInt() );
			player->SetSkin(    p->GetStr("skin")->Get()    );
			player->SetClass(   p->GetStr("class")->Get()   );
			player->SetNick(    p->GetStr("nick")->Get()    );
			player->SetProfile( p->GetStr("profile")->Get() );
		}

		for( size_t i = 0; i < g_conf.dm_bots->GetSize(); ++i )
		{
			ConfVarTable *p = g_conf.dm_bots->GetAt(i)->AsTable();
			GC_PlayerAI *bot = new GC_PlayerAI();
			bot->SetTeam(  p->GetNum("team")->GetInt() );
			bot->SetSkin(  p->GetStr("skin")->Get()    );
			bot->SetClass( p->GetStr("class")->Get()   );
			bot->SetNick(  p->GetStr("nick")->Get()    );
			bot->SetLevel( p->GetNum("level", 2)->GetInt() );
		}
	}
	else
	{
		SAFE_DELETE(g_level);
//		MessageBoxT(NULL, "������ ��� �������� �����", MB_OK|MB_ICONHAND);
		return;
	}

	Close(_resultOK);
}

void NewGameDlg::OnCancel()
{
	Close(_resultCancel);
}

void NewGameDlg::OnSelectPlayer(int index)
{
	_removePlayer->Enable( -1 != index );
	_changePlayer->Enable( -1 != index );
}

void NewGameDlg::OnSelectBot(int index)
{
	_removeBot->Enable( -1 != index );
	_changeBot->Enable( -1 != index );
}

void NewGameDlg::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		if( GetManager()->GetFocusWnd() == _players && -1 != _players->GetCurSel() )
			OnEditPlayer();
		else
			OnOK();
		break;
	case VK_INSERT:
		OnAddPlayer();
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////

EditPlayerDlg::EditPlayerDlg(Window *parent, ConfVarTable *info)
  : Dialog(parent, 0, 0, 384, 192)
{
	_ASSERT(info);

	Move((parent->GetWidth() - GetWidth()) * 0.5f, (parent->GetHeight() - GetHeight()) * 0.5f);
	SetEasyMove(true);

	_info = info;


	float x = 64;
	float y =  8;

	_skinPreview = new Window(this, 270, y, NULL);


	//
	// player name field
	//

	new Text(this, 8, y, "���", alignTextLT);
	_name = new Edit(this, x, y-=1, 200);
	_name->SetText( _info->GetStr("nick", "Unnamed")->Get() );


	List *lst; // helps in combo box filling


	//
	// skins combo
	//
	new Text(this, 8, y+=24, "����", alignTextLT);
	_skins = new ComboBox(this, x, y-=1, 200);
	_skins->eventChangeCurSel.bind( &EditPlayerDlg::OnChangeSkin, this );
	lst = _skins->GetList();
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, "skin/", true);
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = lst->AddItem( names[i].c_str() );
		if( names[i] == _info->GetStr("skin")->Get() )
		{
			_skins->SetCurSel(index);
		}
	}
	if( -1 == _skins->GetCurSel() )
	{
		_skins->SetCurSel(0);
	}


	//
	// create and fill the classes list
	//

	new Text(this, 8, y+=24, "�����", alignTextLT);
	_classes = new ComboBox(this, x, y-=1, 200);

	std::pair<string_t, string_t> val;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		val.first = lua_tostring(g_env.L, -2);
		val.second = lua_tostring(g_env.L, -2); //lua_tostring(L, -1);
		_classNames.push_back(val);

		int index = _classes->GetList()->AddItem(val.first.c_str());
		if( val.first == _info->GetStr("class")->Get() )
		{
			_classes->SetCurSel(index);
		}
	}
	if( -1 == _classes->GetCurSel() )
		_classes->SetCurSel(0);


	new Text(this, 8, y+=24, "�������", alignTextLT);
	_teams = new ComboBox(this, x, y-=1, 200);
	lst = _teams->GetList();

	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		char buf[8];
		wsprintf(buf, i ? "%u" : "[���]", i);
		int index = lst->AddItem(buf);
		if( i == _info->GetNum("team")->GetInt() )
		{
			_teams->SetCurSel(index);
		}
	}
	if( -1 == _teams->GetCurSel() )
	{
		_teams->SetCurSel(0);
	}
	lst->AlignHeightToContent();



	//
	// player profile combo
	//

	new Text(this, 8, y+=24, "�������", alignTextLT);
	_profiles = new ComboBox(this, x, y-=1, 200);
	lst = _profiles->GetList();

	std::vector<string_t> profiles;
	g_conf.dm_profiles->GetKeyList(profiles);

	for( size_t i = 0; i < profiles.size(); ++i )
	{
		int index = lst->AddItem(profiles[i].c_str());
		if( profiles[i] == _info->GetStr("profile")->Get() )
		{
			_profiles->SetCurSel(index);
		}
	}
	if( -1 == _profiles->GetCurSel() )
	{
		_profiles->SetCurSel(0);
	}


	//
	// create buttons
	//

	(new Button(this, 176, 160, "OK"))->eventClick.bind(&EditPlayerDlg::OnOK, this);
	(new Button(this, 280, 160, "������"))->eventClick.bind(&EditPlayerDlg::OnCancel, this);
}

void EditPlayerDlg::OnOK()
{
	_info->SetStr("nick",    _name->GetText().c_str() );
	_info->SetStr("skin",    _skins->GetList()->GetItemText(_skins->GetCurSel(), 0).c_str() );
	_info->SetStr("class",   _classes->GetList()->GetItemText(_classes->GetCurSel(), 0).c_str() );
	_info->SetNum("team",    _teams->GetCurSel());
	_info->SetStr("profile", _profiles->GetList()->GetItemText(_profiles->GetCurSel(), 0).c_str() );

	Close(_resultOK);
}

void EditPlayerDlg::OnCancel()
{
	Close(_resultCancel);
}

void EditPlayerDlg::OnChangeSkin(int index)
{
	if( -1 != index )
	{
		_skinPreview->SetTexture( ("skin/" + _skins->GetList()->GetItemText(index, 0)).c_str() );
		_skinPreview->Resize( _skinPreview->GetTextureWidth(), _skinPreview->GetTextureHeight() );
	}
}

///////////////////////////////////////////////////////////////////////////////

EditBotDlg::EditBotDlg(Window *parent, ConfVarTable *info)
: Dialog(parent, 0, 0, 384, 192)
{
	_ASSERT(info);

	Move((parent->GetWidth() - GetWidth()) * 0.5f, (parent->GetHeight() - GetHeight()) * 0.5f);
	SetEasyMove(true);

	_info = info;


	float x = 64;
	float y =  8;

	_skinPreview = new Window(this, 270, y, NULL);


	//
	// player name field
	//

	new Text(this, 8, y, "���", alignTextLT);
	_name = new Edit(this, x, y-=1, 200);
	lua_getglobal(g_env.L, "random_names");          // push table
	lua_pushinteger(g_env.L, g_level->net_rand() % lua_objlen(g_env.L, -1) + 1);  // push key
	lua_gettable(g_env.L, -2);                       // pop key, push value
	_name->SetText( _info->GetStr("nick", lua_tostring(g_env.L, -1))->Get() ); // default nick from random_names
	lua_pop(g_env.L, 2);                             // pop value and table



	List *lst; // helps in combo box filling


	//
	// skins combo
	//
	new Text(this, 8, y+=24, "����", alignTextLT);
	_skins = new ComboBox(this, x, y-=1, 200);
	_skins->eventChangeCurSel.bind( &EditBotDlg::OnChangeSkin, this );
	lst = _skins->GetList();
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, "skin/", true);
	for( size_t i = 0; i < names.size(); ++i )
	{
		int index = lst->AddItem( names[i].c_str() );
		if( names[i] == _info->GetStr("skin")->Get() )
		{
			_skins->SetCurSel(index);
		}
	}
	if( -1 == _skins->GetCurSel() )
	{
		_skins->SetCurSel(0);
	}


	//
	// create and fill the classes list
	//

	new Text(this, 8, y+=24, "�����", alignTextLT);
	_classes= new ComboBox(this, x, y-=1, 200);

	std::pair<string_t, string_t> val;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		val.first = lua_tostring(g_env.L, -2);
		val.second = lua_tostring(g_env.L, -2); //lua_tostring(L, -1);
		_classNames.push_back(val);

		int index = _classes->GetList()->AddItem(val.first.c_str());
		if( val.first == _info->GetStr("class")->Get() )
		{
			_classes->SetCurSel(index);
		}
	}
	if( -1 == _classes->GetCurSel() )
		_classes->SetCurSel(0);


	new Text(this, 8, y+=24, "�������", alignTextLT);
	_teams = new ComboBox(this, x, y-=1, 200);
	lst = _teams->GetList();

	for( int i = 0; i < MAX_TEAMS; ++i )
	{
		char buf[8];
		wsprintf(buf, i ? "%u" : "[���]", i);
		int index = lst->AddItem(buf);
		if( i == _info->GetNum("team")->GetInt() )
		{
			_teams->SetCurSel(index);
		}
	}
	if( -1 == _teams->GetCurSel() )
	{
		_teams->SetCurSel(0);
	}
	lst->AlignHeightToContent();


	//
	// create and fill the levels list
	//

	new Text(this, 8, y+=24, "�������", alignTextLT);
	_levels = new ComboBox(this, x, y-=1, 200);
	lst = _levels->GetList();

	const char levels[][16] = {
		"�������",
		"�������",
		"�������",
		"�������",
		"�������",
	};
	for( int i = 0; i < 5; ++i )
	{
		int index = lst->AddItem(levels[i]);
		if( i == _info->GetNum("level", 2)->GetInt() )
		{
			_levels->SetCurSel(index);
		}
	}
	if( -1 == _levels->GetCurSel() )
	{
		_levels->SetCurSel(2);
	}
	lst->AlignHeightToContent();


	//
	// create buttons
	//

	(new Button(this, 176, 160, "OK"))->eventClick.bind(&EditBotDlg::OnOK, this);
	(new Button(this, 280, 160, "������"))->eventClick.bind(&EditBotDlg::OnCancel, this);
}

void EditBotDlg::OnOK()
{
	_info->SetStr("nick",  _name->GetText().c_str() );
	_info->SetStr("skin",  _skins->GetList()->GetItemText(_skins->GetCurSel(), 0).c_str() );
	_info->SetStr("class", _classes->GetList()->GetItemText(_classes->GetCurSel(), 0).c_str() );
	_info->SetNum("team",  _teams->GetCurSel());
	_info->SetNum("level", _levels->GetCurSel());

	Close(_resultOK);
}

void EditBotDlg::OnCancel()
{
	Close(_resultCancel);
}

void EditBotDlg::OnChangeSkin(int index)
{
	if( -1 != index )
	{
		_skinPreview->SetTexture( ("skin/" + _skins->GetList()->GetItemText(index, 0)).c_str() );
		_skinPreview->Resize( _skinPreview->GetTextureWidth(), _skinPreview->GetTextureHeight() );
	}
}

///////////////////////////////////////////////////////////////////////////////

MessageArea::MessageArea(Window *parent, float x, float y) : Window(parent, x, y, NULL)
{
	_text = new Text(this, 0, 0, "", alignTextLT);
}

void MessageArea::OnTimeStep(float dt)
{
	for( size_t i = 0; i < _lines.size(); ++i )
		_lines[i].time += dt;
	while( !_lines.empty() && _lines.front().time > 5 )
		_lines.pop_front();

	if( _lines.empty() )
	{
		SetTimeStep(false);
		_text->Show(false);
		return;
	}

	string_t str;
	for( size_t i = 0; i < _lines.size(); ++i )
		str.append(_lines[i].str);
	_text->SetText(str.c_str());
}

void MessageArea::puts(const char *text)
{
	Line line;
	line.time = 0;
	line.str = text;
	line.str.append("\n");
	_lines.push_back(line);
	g_console->puts(line.str.c_str());

	SetTimeStep(true);
	_text->Show(true);
}

void MessageArea::Clear()
{
	_lines.clear();
	SetTimeStep(false);
	_text->SetText("");
	_text->Show(false);
}

///////////////////////////////////////////////////////////////////////////////

void ScriptMessageBox::RunScript(int btn)
{
	lua_rawgeti(g_env.L, LUA_REGISTRYINDEX, _handler);
	luaL_unref(g_env.L, LUA_REGISTRYINDEX, _handler);

	lua_pushinteger(g_env.L, btn);
	if( lua_pcall(g_env.L, 1, 0, 0) )
	{
		g_console->printf("%s\n", lua_tostring(g_env.L, -1));
		lua_pop(g_env.L, 1); // pop the error message from the stack
	}

	Destroy();
}

void ScriptMessageBox::OnButton1()
{
	RunScript(1);
}

void ScriptMessageBox::OnButton2()
{
	RunScript(2);
}

void ScriptMessageBox::OnButton3()
{
	RunScript(3);
}

ScriptMessageBox::ScriptMessageBox( Window *parent, int handler, 
                                    const char *text,
                                    const char *btn1,
                                    const char *btn2,
                                    const char *btn3)
  : Window(parent)
{
	_ASSERT(text);
	_ASSERT(btn1);

	SetBorder(true);
	BringToBack();

	_handler = handler;
	_text = new Text(this, 10, 10, text, alignTextLT);

	_button1 = new Button(this, 0, _text->GetTextHeight() + 20, btn1);
	_button1->eventClick.bind(&ScriptMessageBox::OnButton1, this);

	int nbtn = 1 + (btn2 != NULL) + (btn3 != NULL);

	float by = _text->GetTextHeight() + 20;
	float bw = _button1->GetWidth();
	float w = __max(_text->GetTextWidth() + 10, (bw + 10) * (float) nbtn) + 10;
	float h = by + _button1->GetHeight() + 10;


	Resize(w, h);
	Move(ceilf((GetParent()->GetWidth() - w) / 2), ceilf((GetParent()->GetHeight() - h) / 2));

	_button1->Move(w - 10 - _button1->GetWidth(), _button1->GetY());


	if( btn2 )
	{
		_button2 = new Button(this, _button1->GetX() - 10 - bw, by, btn2);
		_button2->eventClick.bind(&ScriptMessageBox::OnButton2, this);
	}
	else
	{
		_button2 = NULL;
	}

	if( btn3 )
	{
		_button3 = new Button(this, _button2->GetX() - 10 - bw, by, btn3);
		_button3->eventClick.bind(&ScriptMessageBox::OnButton3, this);
	}
	else
	{
		_button3 = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
