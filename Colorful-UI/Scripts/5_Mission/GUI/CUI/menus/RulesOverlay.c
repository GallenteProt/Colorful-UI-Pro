enum CUIRulesRPC
{
	CUI_RULES_RPC_REQUEST = 1383694661,
	CUI_RULES_RPC_RESPONSE = 1383694662
}

class CUIRulesSync
{
	static const int RULES_PROTOCOL_VERSION = 1;
	static const int RULES_REQUEST_TIMEOUT_MS = 10000;
	static const string RULES_PROFILE_DIR = "$profile:Colorful-UI";
	static const string RULES_SERVER_PATH = "$profile:Colorful-UI/rules.md";
	static const string RULES_CLIENT_SAVES_DIR = "$saves:Colorful-UI";
	static const string RULES_CACHE_PATH = "$saves:Colorful-UI/rules.md";

	protected static ref array<string> s_LastRulesLines;
	protected static bool s_RequestPending;
	protected static int s_RequestSerial;
	protected static bool s_RPCHandlerRegistered;

	static bool RegisterRPCHandler()
	{
		if (s_RPCHandlerRegistered)
			return true;

		DayZGame dayZGame = GetDayZGame();
		if (!dayZGame)
			return false;

		dayZGame.Event_OnRPC.Insert(OnRPC);
		s_RPCHandlerRegistered = true;
		return true;
	}

	static void UnregisterRPCHandler()
	{
		if (!s_RPCHandlerRegistered)
			return;

		DayZGame dayZGame = GetDayZGame();
		if (dayZGame)
			dayZGame.Event_OnRPC.Remove(OnRPC);

		s_RPCHandlerRegistered = false;
	}

	static void OnRPC(PlayerIdentity sender, Object target, int rpcType, ParamsReadContext ctx)
	{
		if ((rpcType == CUIRulesRPC.CUI_RULES_RPC_REQUEST) && GetGame().IsDedicatedServer())
		{
			HandleServerRequest(sender, ctx);
			return;
		}

		if ((rpcType == CUIRulesRPC.CUI_RULES_RPC_RESPONSE) && !GetGame().IsDedicatedServer())
		{
			HandleServerResponse(ctx);
		}
	}

	static bool CanRequestRulesFromServer()
	{
		if (GetGame().IsDedicatedServer())
			return false;

		MissionGameplay gameplayMission = MissionGameplay.Cast(GetGame().GetMission());
		return (gameplayMission != null) && GetGame().IsMultiplayer();
	}

	static bool RequestRulesFromServer()
	{
		if (!CanRequestRulesFromServer())
			return false;

		if (s_RequestPending)
			return true;

		s_RequestSerial++;
		s_RequestPending = true;

		ScriptRPC requestRpc = new ScriptRPC();
		requestRpc.Write(RULES_PROTOCOL_VERSION);
		requestRpc.Write(s_RequestSerial);
		requestRpc.Send(null, CUIRulesRPC.CUI_RULES_RPC_REQUEST, true, null);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OnRequestTimeout, RULES_REQUEST_TIMEOUT_MS, false, s_RequestSerial);
		return true;
	}

	static void ResetClientRequestState()
	{
		if (GetGame().IsDedicatedServer())
			return;

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(OnRequestTimeout);
		s_RequestPending = false;
	}

	static void OnRequestTimeout(int requestSerial)
	{
		if (!s_RequestPending || (requestSerial != s_RequestSerial))
			return;

		s_RequestPending = false;
		NotifyRequestFailed("Сервер не ответил на запрос правил.");
	}

	static bool EnsureServerRulesFile()
	{
		if (FileExist(RULES_SERVER_PATH))
			return true;

		MakeDirectory(RULES_PROFILE_DIR);
		FileHandle rulesFile = OpenFile(RULES_SERVER_PATH, FileMode.WRITE);
		if (rulesFile == 0)
		{
			Print("[Colorful-UI] Cannot create server rules file: " + RULES_SERVER_PATH);
			return false;
		}

		FPrintln(rulesFile, "# Правила проекта");
		FPrintln(rulesFile, "");
		FPrintln(rulesFile, "Правила пока не настроены.");
		CloseFile(rulesFile);

		Print("[Colorful-UI] Created server rules file: " + RULES_SERVER_PATH);
		return true;
	}

	static void HandleServerRequest(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!GetGame().IsDedicatedServer() || !sender)
			return;

		int protocolVersion;
		int requestSerial;
		if (!ctx.Read(protocolVersion) || !ctx.Read(requestSerial))
			return;

		array<string> rulesLines = new array<string>;
		if (protocolVersion != RULES_PROTOCOL_VERSION)
		{
			SendServerResponse(sender, requestSerial, false, "Версия протокола правил не совпадает.", rulesLines);
			return;
		}

		if (!EnsureServerRulesFile())
		{
			SendServerResponse(sender, requestSerial, false, "Сервер не смог открыть файл правил.", rulesLines);
			return;
		}

		ReadTextFile(RULES_SERVER_PATH, rulesLines);
		if (rulesLines.Count() == 0)
		{
			SendServerResponse(sender, requestSerial, false, "Файл правил на сервере пуст.", rulesLines);
			return;
		}

		SendServerResponse(sender, requestSerial, true, "", rulesLines);
	}

	static void HandleServerResponse(ParamsReadContext ctx)
	{
		if (GetGame().IsDedicatedServer())
			return;

		int protocolVersion;
		int requestSerial;
		bool requestSucceeded;
		string errorMessage;
		array<string> rulesLines;

		if (!ctx.Read(protocolVersion) || !ctx.Read(requestSerial) || !ctx.Read(requestSucceeded) || !ctx.Read(errorMessage) || !ctx.Read(rulesLines))
		{
			NotifyRequestFailed("Получен поврежденный ответ с правилами.");
			return;
		}

		if ((protocolVersion != RULES_PROTOCOL_VERSION) || (requestSerial != s_RequestSerial))
			return;

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(OnRequestTimeout);
		s_RequestPending = false;

		if (!requestSucceeded)
		{
			NotifyRequestFailed(errorMessage);
			return;
		}

		if (!rulesLines || (rulesLines.Count() == 0))
		{
			NotifyRequestFailed("Сервер прислал пустой файл правил.");
			return;
		}

		StoreLastRules(rulesLines);
		SaveRulesToClientCache(rulesLines);

		CUIRulesOverlay overlay = CUIRulesOverlay.GetActiveOverlay();
		if (overlay)
			overlay.SetRulesLines(rulesLines, false);
	}

	protected static void SendServerResponse(PlayerIdentity recipient, int requestSerial, bool requestSucceeded, string errorMessage, array<string> rulesLines)
	{
		ScriptRPC responseRpc = new ScriptRPC();
		responseRpc.Write(RULES_PROTOCOL_VERSION);
		responseRpc.Write(requestSerial);
		responseRpc.Write(requestSucceeded);
		responseRpc.Write(errorMessage);
		responseRpc.Write(rulesLines);
		responseRpc.Send(null, CUIRulesRPC.CUI_RULES_RPC_RESPONSE, true, recipient);
	}

	protected static void NotifyRequestFailed(string reason)
	{
		Print("[Colorful-UI] Rules request failed: " + reason);

		CUIRulesOverlay overlay = CUIRulesOverlay.GetActiveOverlay();
		if (overlay)
			overlay.SetRulesLoadError();
	}

	static bool ReadRulesFromClientCache(array<string> result)
	{
		result.Clear();

		if (s_LastRulesLines && (s_LastRulesLines.Count() > 0))
		{
			for (int cacheIndex = 0; cacheIndex < s_LastRulesLines.Count(); cacheIndex++)
			{
				result.Insert(s_LastRulesLines.Get(cacheIndex));
			}

			return true;
		}

		ReadTextFile(RULES_CACHE_PATH, result);
		if (result.Count() > 0)
		{
			StoreLastRules(result);
			return true;
		}

		return false;
	}

	protected static void ReadTextFile(string path, array<string> result)
	{
		FileHandle readFile = OpenFile(path, FileMode.READ);
		if (readFile == 0)
			return;

		string lineContent;
		while (FGets(readFile, lineContent) != -1)
		{
			result.Insert(lineContent);
		}

		CloseFile(readFile);
	}

	protected static void SaveRulesToClientCache(array<string> rulesLines)
	{
		MakeDirectory(RULES_CLIENT_SAVES_DIR);
		FileHandle writeFile = OpenFile(RULES_CACHE_PATH, FileMode.WRITE);
		if (writeFile == 0)
			return;

		for (int writeIndex = 0; writeIndex < rulesLines.Count(); writeIndex++)
		{
			FPrintln(writeFile, rulesLines.Get(writeIndex));
		}

		CloseFile(writeFile);
	}

	protected static void StoreLastRules(array<string> rulesLines)
	{
		if (!s_LastRulesLines)
			s_LastRulesLines = new array<string>;

		s_LastRulesLines.Clear();
		for (int storeIndex = 0; storeIndex < rulesLines.Count(); storeIndex++)
		{
			s_LastRulesLines.Insert(rulesLines.Get(storeIndex));
		}
	}
}

enum CUIRulesRowKind
{
	CUI_RULES_ROW_BODY,
	CUI_RULES_ROW_SECTION,
	CUI_RULES_ROW_BULLET,
	CUI_RULES_ROW_NOTE,
	CUI_RULES_ROW_DESCRIPTION,
	CUI_RULES_ROW_UPDATE,
	CUI_RULES_ROW_ERROR
}

class CUIRulesOverlay : ScriptedWidgetEventHandler
{
	protected static const string RULES_LOAD_ERROR_TEXT = "Не удалось загрузить. Попробуйте позже. Или посетите наш сайт.";
	protected static const string RULES_UPDATE_NOTE = "Актуальные правила загрузятся при подключении к серверу.";
	protected static const string RULES_ROW_LAYOUT = "Colorful-UI/GUI/layouts/menus/cui.rules_row.layout";

	protected static CUIRulesOverlay s_ActiveOverlay;

	protected Widget m_Root;
	protected Widget m_ContentRoot;
	protected ButtonWidget m_BackdropButton;
	protected ButtonWidget m_CloseButton;
	protected TextWidget m_CloseText;
	protected Widget m_ClosePanel;
	protected Widget m_CloseAccent;
	protected TextWidget m_StatusText;
	protected ScrollWidget m_Scroll;
	protected Widget m_ScrollSkin;
	protected Widget m_ScrollThumb;
	protected float m_ContentHeight;

	void CUIRulesOverlay(Widget parent)
	{
		m_Root = GetGame().GetWorkspace().CreateWidgets("Colorful-UI/GUI/layouts/menus/cui.rules_overlay.layout", parent);
		if (!m_Root)
			return;

		m_BackdropButton = ButtonWidget.Cast(m_Root.FindAnyWidget("RulesBackdropButton"));
		m_CloseButton = ButtonWidget.Cast(m_Root.FindAnyWidget("RulesCloseButton"));
		m_CloseText = TextWidget.Cast(m_Root.FindAnyWidget("RulesCloseText"));
		m_ClosePanel = m_Root.FindAnyWidget("RulesClosePanel");
		m_CloseAccent = m_Root.FindAnyWidget("RulesCloseAccent");
		m_StatusText = TextWidget.Cast(m_Root.FindAnyWidget("RulesStatusText"));
		m_Scroll = ScrollWidget.Cast(m_Root.FindAnyWidget("RulesScroll"));
		m_ScrollSkin = m_Root.FindAnyWidget("RulesScrollSkin");
		m_ScrollThumb = m_Root.FindAnyWidget("RulesScrollThumb");
		m_ContentRoot = m_Root.FindAnyWidget("RulesContentRoot");
		m_Root.SetHandler(this);

		if (m_BackdropButton)
			m_BackdropButton.SetHandler(this);

		if (m_CloseButton)
			m_CloseButton.SetHandler(this);

		m_Root.Show(false);
	}

	static CUIRulesOverlay GetActiveOverlay()
	{
		return s_ActiveOverlay;
	}

	void Open(bool requestFromServer)
	{
		if (!m_Root)
			return;

		s_ActiveOverlay = this;
		m_Root.Show(true);

		array<string> cachedLines = new array<string>;
		bool hasCachedRules = CUIRulesSync.ReadRulesFromClientCache(cachedLines);
		if (hasCachedRules)
		{
			SetRulesLines(cachedLines, true);
		}
		else
		{
			SetStatus("Правила еще не получены с сервера.");
			SetBodyText(RULES_UPDATE_NOTE);
		}

		if (!requestFromServer)
			return;

		if (CUIRulesSync.RequestRulesFromServer())
		{
			SetStatus("Получение правил с сервера...");
			if (!hasCachedRules)
				SetBodyText(RULES_UPDATE_NOTE);
		}
	}

	void Close()
	{
		if (m_Root)
			m_Root.Show(false);

		if (s_ActiveOverlay == this)
			s_ActiveOverlay = null;
	}

	bool IsOpen()
	{
		if (!m_Root)
			return false;

		return m_Root.IsVisible();
	}

	void Dispose()
	{
		if (m_BackdropButton)
			m_BackdropButton.SetHandler(null);

		if (m_CloseButton)
			m_CloseButton.SetHandler(null);

		if (s_ActiveOverlay == this)
			s_ActiveOverlay = null;

		m_BackdropButton = null;
		m_CloseButton = null;
		m_CloseText = null;
		m_ClosePanel = null;
		m_CloseAccent = null;
		m_StatusText = null;
		m_Scroll = null;
		m_ScrollSkin = null;
		m_ScrollThumb = null;
		m_ContentRoot = null;

		if (m_Root)
		{
			m_Root.SetHandler(null);
			m_Root.Unlink();
			m_Root = null;
		}
	}

	void SetRulesLines(array<string> rulesLines, bool fromCache)
	{
		if (fromCache)
			SetStatus("Показана сохраненная копия правил сервера.");
		else
			SetStatus("Правила получены с сервера.");

		RenderRules(rulesLines);
	}

	void SetRulesLoadError()
	{
		array<string> cachedLines = new array<string>;
		if (CUIRulesSync.ReadRulesFromClientCache(cachedLines))
		{
			SetStatus(RULES_LOAD_ERROR_TEXT);
			RenderRules(cachedLines);
			return;
		}

		SetStatus("");
		RenderMessage(RULES_LOAD_ERROR_TEXT, CUIRulesRowKind.CUI_RULES_ROW_ERROR, true);
	}

	protected void SetStatus(string statusText)
	{
		if (m_StatusText)
			m_StatusText.SetText(statusText);
	}

	protected void SetBodyText(string bodyText)
	{
		RenderMessage(bodyText, CUIRulesRowKind.CUI_RULES_ROW_UPDATE, false);
	}

	protected void RenderRules(array<string> rulesLines)
	{
		ClearRuleRows();

		if (!m_ContentRoot)
			return;

		if (!rulesLines || (rulesLines.Count() == 0))
		{
			AddRuleRow("Правила пока не настроены.", CUIRulesRowKind.CUI_RULES_ROW_ERROR, false);
			AddRuleRow(RULES_UPDATE_NOTE, CUIRulesRowKind.CUI_RULES_ROW_UPDATE, true);
			RefreshRulesLayout();
			return;
		}

		bool nextLineIsSection;
		bool pendingSpacing;
		int renderedLineCount;
		for (int lineIndex = 0; lineIndex < rulesLines.Count(); lineIndex++)
		{
			string sourceLine = rulesLines.Get(lineIndex) + "";
			sourceLine.Replace("\r", "");
			sourceLine.TrimInPlace();

			if (sourceLine == "")
			{
				pendingSpacing = true;
				continue;
			}

			if (IsMarkdownDivider(sourceLine))
			{
				if (sourceLine.IndexOf("#") != -1)
					nextLineIsSection = true;

				pendingSpacing = true;
				continue;
			}

			bool explicitSection = sourceLine.IndexOf("#") == 0;
			bool bulletLine = sourceLine.Length() >= 2 && sourceLine.Substring(0, 2) == "- ";
			string displayLine = SanitizeMarkdownLine(sourceLine);
			if (displayLine == "")
			{
				pendingSpacing = true;
				continue;
			}

			int rowKind = GetRowKind(sourceLine, displayLine, explicitSection, nextLineIsSection, bulletLine);

			AddRuleRow(displayLine, rowKind, pendingSpacing && (renderedLineCount > 0));
			renderedLineCount++;
			nextLineIsSection = false;
			pendingSpacing = false;
		}

		AddRuleRow(RULES_UPDATE_NOTE, CUIRulesRowKind.CUI_RULES_ROW_UPDATE, true);
		RefreshRulesLayout();
	}

	protected void RenderMessage(string messageText, int rowKind, bool appendUpdateNote)
	{
		ClearRuleRows();

		string cleanMessage = SanitizeMarkdownLine(messageText);
		if (cleanMessage != "")
			AddRuleRow(cleanMessage, rowKind, false);

		if (appendUpdateNote)
			AddRuleRow(RULES_UPDATE_NOTE, CUIRulesRowKind.CUI_RULES_ROW_UPDATE, true);

		RefreshRulesLayout();
	}

	protected void ClearRuleRows()
	{
		m_ContentHeight = 0;

		if (!m_ContentRoot)
			return;

		Widget childWidget = m_ContentRoot.GetChildren();
		while (childWidget)
		{
			Widget widgetToDelete = childWidget;
			childWidget = childWidget.GetSibling();
			widgetToDelete.Unlink();
		}
	}

	protected void AddRuleRow(string displayText, int rowKind, bool addTopSpacing)
	{
		if (!m_ContentRoot || displayText == "")
			return;

		Widget rowRoot = GetGame().GetWorkspace().CreateWidgets(RULES_ROW_LAYOUT, m_ContentRoot);
		if (!rowRoot)
			return;

		RichTextWidget rowText = RichTextWidget.Cast(rowRoot.FindAnyWidget("RulesRowText"));
		Widget rowSurface = rowRoot.FindAnyWidget("RulesRowSurface");
		Widget rowAccent = rowRoot.FindAnyWidget("RulesRowAccent");
		Widget rowDivider = rowRoot.FindAnyWidget("RulesRowDivider");
		Widget rowBulletDot = rowRoot.FindAnyWidget("RulesRowBulletDot");
		if (!rowText)
		{
			rowRoot.Unlink();
			return;
		}

		int textSize = 17;
		int textColor = colorScheme.PrimaryText();
		int accentColor = UIColor.ZoneLoadingBlue();
		int surfaceColor = ARGB(70, 13, 23, 33);
		float verticalPadding = 7;
		float minimumTextHeight = 22;
		bool showSurface;
		bool showAccent;
		bool showDivider = true;
		bool useBold;
		bool useItalic;

		switch (rowKind)
		{
			case CUIRulesRowKind.CUI_RULES_ROW_SECTION:
				textSize = 24;
				textColor = UIColor.ZoneText();
				accentColor = UIColor.ZoneLoadingBlue();
				surfaceColor = ARGB(165, 13, 23, 33);
				verticalPadding = 11;
				minimumTextHeight = 30;
				showSurface = true;
				showAccent = true;
				showDivider = false;
				useBold = true;
				break;

			case CUIRulesRowKind.CUI_RULES_ROW_BULLET:
				textColor = UIColor.ZoneTextMuted();
				accentColor = ARGB(175, 82, 156, 210);
				surfaceColor = ARGB(78, 7, 16, 24);
				verticalPadding = 8;
				showSurface = true;
				showAccent = true;
				showDivider = false;
				break;

			case CUIRulesRowKind.CUI_RULES_ROW_NOTE:
				textColor = UIColor.ZoneText();
				accentColor = UIColor.ZoneAgedPaper();
				surfaceColor = ARGB(112, 28, 39, 48);
				verticalPadding = 9;
				showSurface = true;
				showAccent = true;
				showDivider = false;
				useBold = true;
				break;

			case CUIRulesRowKind.CUI_RULES_ROW_DESCRIPTION:
				textColor = UIColor.ZoneTextMuted();
				verticalPadding = 8;
				showDivider = false;
				useItalic = true;
				break;

			case CUIRulesRowKind.CUI_RULES_ROW_UPDATE:
				textSize = 15;
				textColor = UIColor.ZoneTextDim();
				accentColor = UIColor.ZoneBorder();
				surfaceColor = ARGB(64, 7, 16, 24);
				verticalPadding = 9;
				showSurface = true;
				showAccent = true;
				showDivider = false;
				useItalic = true;
				break;

			case CUIRulesRowKind.CUI_RULES_ROW_ERROR:
				textSize = 18;
				textColor = UIColor.ZoneText();
				accentColor = UIColor.ZoneDanger();
				surfaceColor = ARGB(105, 72, 22, 22);
				verticalPadding = 11;
				showSurface = true;
				showAccent = true;
				showDivider = false;
				useBold = true;
				break;
		}

		float extraTopSpacing;
		if (addTopSpacing)
			extraTopSpacing = 10;

		rowText.SetTextExactSize(textSize);
		rowText.SetColor(textColor);
		rowText.SetBold(useBold);
		rowText.SetItalic(useItalic);
		rowText.SetText(displayText);

		float textPositionX;
		float textPositionY;
		rowText.GetPos(textPositionX, textPositionY);
		if (rowKind == CUIRulesRowKind.CUI_RULES_ROW_BULLET)
			textPositionX = 32;

		rowText.SetPos(textPositionX, verticalPadding + extraTopSpacing);
		rowRoot.Update();
		rowText.Update();

		float contentHeight = rowText.GetContentHeight();
		int visibleLineCount = rowText.GetNumLines();
		float calculatedLineHeight = visibleLineCount * (textSize + 5);
		if (calculatedLineHeight > contentHeight)
			contentHeight = calculatedLineHeight;

		if (contentHeight < minimumTextHeight)
			contentHeight = minimumTextHeight;

		float textWidth;
		float textHeight;
		rowText.GetSize(textWidth, textHeight);
		rowText.SetSize(textWidth, contentHeight + 2);

		float rowWidth;
		float rowHeight;
		rowRoot.GetSize(rowWidth, rowHeight);
		float finalRowHeight = contentHeight + (verticalPadding * 2) + extraTopSpacing;
		rowRoot.SetPos(0, m_ContentHeight);
		rowRoot.SetSize(rowWidth, finalRowHeight);
		m_ContentHeight += finalRowHeight + 5;

		if (rowSurface)
		{
			rowSurface.SetColor(surfaceColor);
			rowSurface.Show(showSurface);
		}

		if (rowAccent)
		{
			rowAccent.SetColor(accentColor);
			rowAccent.Show(showAccent);
		}

		if (rowDivider)
			rowDivider.Show(showDivider);

		if (rowBulletDot && (rowKind == CUIRulesRowKind.CUI_RULES_ROW_BULLET))
		{
			float bulletPositionX;
			float bulletPositionY;
			float bulletWidth;
			float bulletHeight;
			rowBulletDot.GetPos(bulletPositionX, bulletPositionY);
			rowBulletDot.GetSize(bulletWidth, bulletHeight);
			rowBulletDot.SetPos(bulletPositionX, (finalRowHeight - bulletHeight) * 0.5);
			rowBulletDot.SetColor(accentColor);
			rowBulletDot.Show(true);
		}
	}

	protected void RefreshRulesLayout()
	{
		if (m_Root)
			m_Root.Update();

		if (m_ContentRoot)
		{
			float contentWidth;
			float oldContentHeight;
			m_ContentRoot.GetSize(contentWidth, oldContentHeight);
			float finalContentHeight = m_ContentHeight + 10;
			if (finalContentHeight < 300)
				finalContentHeight = 300;

			m_ContentRoot.SetPos(0, 0);
			m_ContentRoot.SetSize(contentWidth, finalContentHeight);
			m_ContentRoot.Update();
		}

		if (m_Scroll)
		{
			m_Scroll.Update();
			m_Scroll.VScrollToPos01(0);
		}

		UpdateScrollSkin();
	}

	protected void UpdateScrollSkin()
	{
		if (!m_ScrollSkin || !m_ScrollThumb || !m_Scroll || !m_ContentRoot)
			return;

		float scrollWidth;
		float scrollHeight;
		float contentWidth;
		float contentHeight;
		m_Scroll.GetScreenSize(scrollWidth, scrollHeight);
		m_ContentRoot.GetScreenSize(contentWidth, contentHeight);

		bool canScroll = (scrollHeight > 0) && (contentHeight > scrollHeight + 1);
		m_ScrollSkin.Show(canScroll);
		if (!canScroll)
			return;

		float skinWidth;
		float skinHeight;
		m_ScrollSkin.GetScreenSize(skinWidth, skinHeight);
		if (skinHeight <= 0)
			return;

		float thumbHeight = skinHeight * (scrollHeight / contentHeight);
		if (thumbHeight < 42)
			thumbHeight = 42;
		else if (thumbHeight > skinHeight)
			thumbHeight = skinHeight;

		float thumbPositionX;
		float oldThumbPositionY;
		float thumbWidth;
		float oldThumbHeight;
		m_ScrollThumb.GetPos(thumbPositionX, oldThumbPositionY);
		m_ScrollThumb.GetSize(thumbWidth, oldThumbHeight);
		m_ScrollThumb.SetPos(thumbPositionX, (skinHeight - thumbHeight) * m_Scroll.GetVScrollPos01());
		m_ScrollThumb.SetSize(thumbWidth, thumbHeight);
	}

	protected int GetRowKind(string sourceLine, string displayLine, bool explicitSection, bool nextLineIsSection, bool bulletLine)
	{
		if (explicitSection || nextLineIsSection)
			return CUIRulesRowKind.CUI_RULES_ROW_SECTION;

		if (bulletLine)
			return CUIRulesRowKind.CUI_RULES_ROW_BULLET;

		string lowerLine = displayLine + "";
		lowerLine.ToLower();
		if (lowerLine.IndexOf("исключ") == 0 || lowerLine.IndexOf("примечан") == 0 || lowerLine.IndexOf("важно") == 0 || lowerLine.IndexOf("пример") == 0 || lowerLine.IndexOf("внимание") == 0)
			return CUIRulesRowKind.CUI_RULES_ROW_NOTE;

		if (sourceLine.Length() >= 3 && sourceLine.Substring(0, 3) == "***" && !StartsWithNumber(displayLine))
			return CUIRulesRowKind.CUI_RULES_ROW_DESCRIPTION;

		return CUIRulesRowKind.CUI_RULES_ROW_BODY;
	}

	protected bool StartsWithNumber(string lineText)
	{
		if (lineText.Length() == 0)
			return false;

		return "0123456789".IndexOf(lineText.Substring(0, 1)) != -1;
	}

	protected bool IsMarkdownDivider(string sourceLine)
	{
		string dividerTest = sourceLine + "";
		dividerTest.Replace("#", "");
		dividerTest.Replace("-", "");
		dividerTest.Replace("*", "");
		dividerTest.Replace("_", "");
		dividerTest.TrimInPlace();
		return dividerTest == "";
	}

	protected string SanitizeMarkdownLine(string sourceLine)
	{
		string lineText = sourceLine + "";
		lineText.Replace("\r", "");
		lineText.Replace("\t", " ");
		lineText.Replace(" ", " ");
		lineText.Replace("<u>", "");
		lineText.Replace("</u>", "");
		lineText.Replace("<br>", " ");
		lineText.Replace("<br/>", " ");
		lineText.Replace("<br />", " ");
		lineText.Replace("&nbsp;", " ");
		lineText.Replace("*", "");
		lineText.Replace("`", "");

		while (lineText.IndexOf("#") == 0)
		{
			lineText = lineText.Substring(1, lineText.Length() - 1);
			lineText.TrimInPlace();
		}

		if (lineText.Length() >= 2 && lineText.Substring(0, 2) == "- ")
			lineText = lineText.Substring(2, lineText.Length() - 2);

		if (lineText.Length() >= 2 && lineText.Substring(0, 2) == "> ")
			lineText = lineText.Substring(2, lineText.Length() - 2);

		int linkLabelStart = lineText.IndexOf("[");
		while (linkLabelStart != -1)
		{
			int linkLabelEnd = lineText.IndexOfFrom(linkLabelStart, "](");
			if (linkLabelEnd == -1)
				break;

			int linkUrlEnd = lineText.IndexOfFrom(linkLabelEnd + 2, ")");
			if (linkUrlEnd == -1)
				break;

			string linkPrefix = lineText.Substring(0, linkLabelStart);
			string linkLabel = lineText.Substring(linkLabelStart + 1, linkLabelEnd - linkLabelStart - 1);
			string linkSuffix = lineText.Substring(linkUrlEnd + 1, lineText.Length() - linkUrlEnd - 1);
			lineText = linkPrefix + linkLabel + linkSuffix;
			linkLabelStart = lineText.IndexOf("[");
		}

		lineText.Replace("[", "");
		lineText.Replace("]", "");
		lineText.Replace("“", "\"");
		lineText.Replace("”", "\"");
		while (lineText.Replace("  ", " ") > 0)
		{
		}

		lineText.TrimInPlace();
		return lineText;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
			return false;

		if ((w == m_CloseButton) || (w == m_BackdropButton))
		{
			Close();
			return true;
		}

		return false;
	}

	override bool OnUpdate(Widget w)
	{
		if (w == m_Root)
		{
			UpdateScrollSkin();
			return true;
		}

		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w == m_CloseButton)
		{
			if (m_ClosePanel)
				m_ClosePanel.SetColor(ARGB(255, 13, 44, 64));

			if (m_CloseAccent)
				m_CloseAccent.SetColor(UIColor.ZoneActiveText());

			if (m_CloseText)
				m_CloseText.SetColor(UIColor.ZoneActiveText());

			return true;
		}

		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_CloseButton)
		{
			if (m_ClosePanel)
				m_ClosePanel.SetColor(ARGB(245, 7, 16, 24));

			if (m_CloseAccent)
				m_CloseAccent.SetColor(UIColor.ZoneLoadingBlue());

			if (m_CloseText)
				m_CloseText.SetColor(colorScheme.PrimaryText());

			return true;
		}

		return false;
	}
}

modded class MissionGameplay
{
	override void OnMissionStart()
	{
		super.OnMissionStart();

		CUIRulesSync.RegisterRPCHandler();
		CUIRulesSync.ResetClientRequestState();
		CUIRulesSync.RequestRulesFromServer();
	}

	override void OnMissionFinish()
	{
		CUIRulesSync.ResetClientRequestState();
		CUIRulesSync.UnregisterRPCHandler();
		super.OnMissionFinish();
	}

	override void OnPlayerRespawned(Man player)
	{
		super.OnPlayerRespawned(player);

		GetGame().GetUIManager().ScreenFadeOut(0);

		PlayerBase playerBase = PlayerBase.Cast(player);
		if (playerBase)
		{
			playerBase.StopDeathDarkeningEffect();
		}
	}
}

modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		CUIRulesSync.RegisterRPCHandler();
		CUIRulesSync.EnsureServerRulesFile();
	}
}
