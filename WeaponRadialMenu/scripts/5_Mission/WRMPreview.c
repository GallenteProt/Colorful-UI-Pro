// Client-side 3D preview objects of ONE wheel instance (each WRMMenu owns its own set, so a late destructor of an
// old menu can never delete the objects of the menu that replaced it).
//  - objects are local (ECE_LOCAL, same flags as vanilla ScriptConsoleItemsTab.c:696), have no physics and no
//    simulation, and are never sent to the server;
//  - an object lives only while a card shows it: BeginFrame()/EndFrame() around a card rebuild delete the ones that
//    are no longer used, so only the current page exists and no preview widget points at a deleted entity;
//  - only entities created here are ever deleted; entities of the world (item in hands, installed attachment) are
//    shown directly by the card and never pass through this class;
//  - classes that fail to spawn are remembered and not retried (the card shows its generic icon).
class WRMPreviewSet
{
	static ref TStringArray s_Failed = new TStringArray;

	protected ref map<string, EntityAI> m_Objects = new map<string, EntityAI>;
	protected ref map<string, bool> m_Used = new map<string, bool>;

	void ~WRMPreviewSet()
	{
		Clear();
	}

	// Marks every object as unused; Get() marks the ones a new card asks for.
	void BeginFrame()
	{
		m_Used.Clear();
	}

	// Destroys the objects no card asked for since BeginFrame(). Call after the old cards were deleted.
	void EndFrame()
	{
		TStringArray doomed = new TStringArray;
		string key;
		for (int i = 0; i < m_Objects.Count(); i++)
		{
			key = m_Objects.GetKey(i);
			if (!m_Used.Contains(key))
				doomed.Insert(key);
		}
		for (int j = 0; j < doomed.Count(); j++)
		{
			key = doomed.Get(j);
			EntityAI obj = m_Objects.Get(key);
			m_Objects.Remove(key);
			if (obj)
				obj.Delete();
		}
	}

	// Deletes everything this set created. Call after the cards were deleted (hide, close, destructor).
	void Clear()
	{
		for (int i = 0; i < m_Objects.Count(); i++)
		{
			EntityAI obj = m_Objects.GetElement(i);
			if (obj && g_Game)
				g_Game.ObjectDelete(obj);
		}
		m_Objects.Clear();
		m_Used.Clear();
	}

	EntityAI Get(string className)
	{
		if (className == "")
			return null;

		EntityAI cached;
		if (m_Objects.Find(className, cached) && cached)
		{
			m_Used.Set(className, true);
			return cached;
		}

		string lower = className;
		lower.ToLower();
		if (s_Failed.Find(lower) != -1 || !g_Game)
			return null;

#ifdef DEVELOPER
		DayZGame.m_IsPreviewSpawn = true;		// vanilla: no weapon FSM errors for a preview-only spawn
#endif
		EntityAI obj = EntityAI.Cast(g_Game.CreateObjectEx(className, "0 0 0", ECE_LOCAL | ECE_CREATEPHYSICS | ECE_TRACE));
#ifdef DEVELOPER
		DayZGame.m_IsPreviewSpawn = false;
#endif

		if (!obj)
		{
			s_Failed.Insert(lower);
			return null;
		}

		dBodyDestroy(obj);		// preview only: no physics
		obj.DisableSimulation(true);
		obj.SetAllowDamage(false);
		m_Objects.Set(className, obj);
		m_Used.Set(className, true);
		return obj;
	}
}
