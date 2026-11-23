// One envelope for every request and reply. Reads are bounded: counts and string lengths are checked
// before anything is stored, so a malicious payload cannot make the server allocate or loop unbounded.
class WRMMsg
{
	static const int MAX_INTS = 32;
	static const int MAX_STRS = 48;
	static const int MAX_STR_LEN = 96;

	int m_Op;
	int m_Status;
	ref TIntArray m_Ints = new TIntArray;
	ref TStringArray m_Strs = new TStringArray;

	void WRMMsg(int op = 0, int status = 0)
	{
		m_Op = op;
		m_Status = status;
	}

	int GetInt(int index, int fallback = 0)
	{
		if (index < 0 || index >= m_Ints.Count())
			return fallback;
		return m_Ints.Get(index);
	}

	string GetStr(int index)
	{
		if (index < 0 || index >= m_Strs.Count())
			return "";
		return m_Strs.Get(index);
	}

	void Write(ParamsWriteContext ctx)
	{
		ctx.Write(WRM_MAGIC);
		ctx.Write(m_Op);
		ctx.Write(m_Status);

		int intCount = m_Ints.Count();
		if (intCount > MAX_INTS)
			intCount = MAX_INTS;
		ctx.Write(intCount);
		for (int i = 0; i < intCount; i++)
			ctx.Write(m_Ints.Get(i));

		int strCount = m_Strs.Count();
		if (strCount > MAX_STRS)
			strCount = MAX_STRS;
		ctx.Write(strCount);
		for (int j = 0; j < strCount; j++)
			ctx.Write(m_Strs.Get(j));
	}

	bool Read(ParamsReadContext ctx)
	{
		int magic;
		if (!ctx.Read(magic) || magic != WRM_MAGIC)
			return false;

		int op;
		int status;
		if (!ctx.Read(op) || !ctx.Read(status))
			return false;
		m_Op = op;
		m_Status = status;

		int intCount;
		if (!ctx.Read(intCount) || intCount < 0 || intCount > MAX_INTS)
			return false;
		for (int i = 0; i < intCount; i++)
		{
			int value;
			if (!ctx.Read(value))
				return false;
			m_Ints.Insert(value);
		}

		int strCount;
		if (!ctx.Read(strCount) || strCount < 0 || strCount > MAX_STRS)
			return false;
		for (int j = 0; j < strCount; j++)
		{
			string text;
			if (!ctx.Read(text) || text.Length() > MAX_STR_LEN)
				return false;
			m_Strs.Insert(text);
		}

		return true;
	}
}
