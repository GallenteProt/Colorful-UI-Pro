// Constants.c v3.0.0
static bool StartMainMenu      = true;  // Если true, при запуске принудительно показывается главное меню.
static bool NoHints			   = false;  // Если true, подсказки не показываются на экранах загрузки.
static bool UseImagesets       = false;   // Если true, записи hints.json с m_ImageSet/m_ImageName загружаются из зарегистрированного imageset backgrounds; иначе используется m_ImagePath.
static bool LoadVideo          = false;  // Видео удалены из мода, оставлять false.

// Параметры ниже сейчас часто ломаются. Исправление планируется в обновлении 4.0.0.
static bool EnableMenuVideo    = false;  // Видео удалены из мода, оставлять false.
static bool EnableOptionsVideo = false;  // Видео удалены из мода, оставлять false.

// Информация о сервере
// (В кнопках CUI возможны баги. На живых серверах ещё не тестировалось)
static const string SERVER_IP = "37.230.228.247";
static const int SERVER_PORT = 2302;
static const string DIAG_SERVER_IP = "127.0.0.1";
static const int DIAG_SERVER_PORT = 2302;

// Настройки видео (меняйте по своему усмотрению)
static const string m_LoadingVideo     = "";
static const string m_MainMenuVideo    = "";
static const string m_OptionsMenuVideo = "";

static const string CUI_FACTION_PROFILE_DIR = "$profile:Colorful-UI/";
static const string CUI_FACTION_SAVES_DIR = "$saves:Colorful-UI/";
static const string CUI_FACTION_BACKGROUND_DIR = "Colorful-UI/GUI/textures/Factions/";

class LonelinessFactionMenuBackgrounds
{
	static string ExtractFactionIdFromFileName(string fileName)
	{
		int dotIndex = fileName.LastIndexOf(".");
		if (dotIndex <= 0)
			return "";

		string extension = fileName.Substring(dotIndex, fileName.Length() - dotIndex);
		extension.ToLower();
		if (extension != ".txt")
			return "";

		return fileName.Substring(0, dotIndex);
	}

	static string FindFactionIdFromDir(string dirPath)
	{
		string foundFileName = "";
		FileAttr foundFileAttr;
		FindFileHandle findHandle = FindFile(dirPath + "*.txt", foundFileName, foundFileAttr, 0);
		if (!findHandle)
			return "";

		bool foundFile = true;
		string factionId = "";
		while (foundFile)
		{
			factionId = ExtractFactionIdFromFileName(foundFileName);
			if (factionId != "")
			{
				CloseFindFile(findHandle);
				return factionId;
			}

			foundFile = FindNextFile(findHandle, foundFileName, foundFileAttr);
		}

		CloseFindFile(findHandle);

		return "";
	}

	static string ReadFactionId()
	{
		string savesFactionId = FindFactionIdFromDir(CUI_FACTION_SAVES_DIR);
		if (savesFactionId != "")
			return savesFactionId;

		string profileFactionId = FindFactionIdFromDir(CUI_FACTION_PROFILE_DIR);
		if (profileFactionId != "")
			return profileFactionId;

		return "";
	}

	static string GetFactionFindPattern(string factionId)
	{
		return CUI_FACTION_BACKGROUND_DIR + factionId + "_*.edds";
	}

	static string GetFactionBackgroundPath(string foundFileName)
	{
		string backgroundFileName = foundFileName;

		int slashIndex = backgroundFileName.LastIndexOf("/");
		if (slashIndex >= 0)
			return backgroundFileName;

		return CUI_FACTION_BACKGROUND_DIR + backgroundFileName;
	}
}

class LonelinessFactionBackgroundPool
{
	static string m_CurrentFactionId;
	static ref array<string> m_RemainingFactionBackgrounds;

	static void FillPool(string factionId)
	{
		if (!m_RemainingFactionBackgrounds)
			m_RemainingFactionBackgrounds = new array<string>;

		if (m_CurrentFactionId != factionId)
		{
			m_CurrentFactionId = factionId;
			m_RemainingFactionBackgrounds.Clear();
		}

		if (m_RemainingFactionBackgrounds.Count() > 0)
			return;

		string foundFileName = "";
		FileAttr foundFileAttr;
		string findPattern = LonelinessFactionMenuBackgrounds.GetFactionFindPattern(factionId);
		FindFileHandle findHandle = FindFile(findPattern, foundFileName, foundFileAttr, 0);
		if (!findHandle)
			return;

		bool foundFile = true;
		while (foundFile)
		{
			string factionBackgroundPath = LonelinessFactionMenuBackgrounds.GetFactionBackgroundPath(foundFileName);
			m_RemainingFactionBackgrounds.Insert(factionBackgroundPath);

			foundFile = FindNextFile(findHandle, foundFileName, foundFileAttr);
		}

		CloseFindFile(findHandle);
	}

	static string TakeNextFactionBackground(string factionId)
	{
		if (factionId == "")
			return "";

		FillPool(factionId);

		if (!m_RemainingFactionBackgrounds || m_RemainingFactionBackgrounds.Count() == 0)
			return "";

		int randomFactionPoolIndex = Math.RandomInt(0, m_RemainingFactionBackgrounds.Count());
		string factionBackgroundPath = m_RemainingFactionBackgrounds.Get(randomFactionPoolIndex);
		m_RemainingFactionBackgrounds.RemoveOrdered(randomFactionPoolIndex);
		return factionBackgroundPath;
	}
}

// Фон главного меню: faction-фон при наличии локального id-файла,
// иначе обычная ротация из шести Loneliness_Loadscreen.
string GetMainMenuBackground()
{
	string factionId = LonelinessFactionMenuBackgrounds.ReadFactionId();
	string factionBackground = LonelinessFactionBackgroundPool.TakeNextFactionBackground(factionId);
	if (factionBackground != "")
		return factionBackground;

	return GetRandomLonelinessBackground();
}

class LonelinessBackgroundPool
{
	static ref array<int> m_RemainingBackgrounds;

	static void FillPool()
	{
		if (!m_RemainingBackgrounds)
			m_RemainingBackgrounds = new array<int>;

		if (m_RemainingBackgrounds.Count() > 0)
			return;

		for (int bgPoolIndex = 1; bgPoolIndex <= 6; bgPoolIndex++)
		{
			m_RemainingBackgrounds.Insert(bgPoolIndex);
		}
	}

	static int TakeNextIndex()
	{
		FillPool();

		int randomPoolIndex = Math.RandomInt(0, m_RemainingBackgrounds.Count());
		int backgroundIndex = m_RemainingBackgrounds.Get(randomPoolIndex);
		m_RemainingBackgrounds.RemoveOrdered(randomPoolIndex);
		return backgroundIndex;
	}
}

string GetRandomLonelinessBackground()
{
    int loadscreenIndex = LonelinessBackgroundPool.TakeNextIndex();
    return "Colorful-UI/GUI/textures/LoadScreens/Loneliness_Loadscreen_" + loadscreenIndex.ToString() + "_co.paa";
}

// Фон экранов загрузки (3 фазы + ротационные подсказки) — отдельно от
// главного меню. Сырой .edds: bg.imageset непригоден (пустой блок Images{}).
string GetLoadingBackground()
{
    return GetRandomLonelinessBackground();
}
// Логотип сервера
// ПРИМЕЧАНИЕ: настройки ширины и высоты здесь пока не реализованы.
//        Рекомендуемый размер — 512×512; пока размер задаётся вручную в layout-файле.
class Branding
{
    static string Logo()
    {
        return "Colorful-UI/GUI/textures/Shared/Loneliness_Logo.paa";
    }
    static void ApplyLogo(ImageWidget widget)
    {
        if (!widget) return;
        widget.LoadImageFile(0, Logo());
        widget.SetFlags(WidgetFlags.STRETCH);
    }
};

// URL ссылок (добавьте свои ссылки здесь)
// ПРИМЕЧАНИЕ: если кнопка не нужна, укажите URL «#» или «».
//        Если не задать — кнопки не появятся в layout после компиляции.
//        Так отображаются только нужные кнопки.

class CustomURL {
	static string Website    = "https://loneliness-rp.ru/";
	static string PriorityQ  = "#";
	static string Custom     = "#";
	static string Rules      = "#";
	static string Lore       = "#";
	static string Shop       = "#";
	static string MusicAuthor = "https://joemathews.bandcamp.com";
}

class SocialURL {
	static string Discord    = "https://discord.gg/3UNhzu2Re9";
	static string Facebook   = "#";
	static string Twitter    = "#";
	static string Reddit     = "#";
	static string Youtube    = "#";
}

