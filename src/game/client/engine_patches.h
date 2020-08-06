#ifndef ENGINE_PATCHES_H
#define ENGINE_PATCHES_H
#include <cstdint>

using SvcParseFunc = void (*)();

struct SvcHandler
{
	unsigned char nOpcode;
	const char *pszName;
	SvcParseFunc *pfnParse;
};

struct UserMessage
{
	int messageId;
	int messageLen;
	char messageName[16];
	UserMessage *nextMessage;
};

/**
 * Engine patching includes fixing some bugs and 
 */
class CEnginePatches
{
public:
	struct EngineMsgBuf
	{
		inline bool IsValid() const
		{
			return m_EngineBuf;
		}

		inline void *GetBuf() const { return *m_EngineBuf; }
		inline int &GetSize() const { return *m_EngineBufSize; }
		inline int &GetReadPos() const { return *m_EngineReadPos; }

		void **m_EngineBuf = nullptr;
		int *m_EngineBufSize = nullptr;
		int *m_EngineReadPos = nullptr;
	};

	/**
	 * Returns singleton instance.
	 */
	static CEnginePatches &Get();

	CEnginePatches();
	CEnginePatches(const CEnginePatches &) = delete;
	CEnginePatches &operator=(const CEnginePatches &) = delete;
	virtual ~CEnginePatches();

	/**
	 * Enables engine patches.
	 */
	void Init();

	/**
	 * Unpatches the engine.
	 */
	void Shutdown();

	/**
	 * Called every frame.
	 */
	virtual void RunFrame();

	/**
	 * Returns truf if client running SteamPipe version of engine which uses SDL2.
	 */
	bool IsSDLEngine();

	/**
	 * Returns engine message buffer.
	 * Make sure to call IsValid() before use.
	 */
	const EngineMsgBuf &GetMsgBuf();

	/**
	 * Returns an array of SVC message handlers or nullptr.
	 */
	SvcHandler *GetSvcArray();

	/**
	 * Returns first element of user message linked list or nullptr.
	 */
	UserMessage *GetUserMsgList();

	//-------------------------------------------------------------------
	// Utility functions
	//-------------------------------------------------------------------
	/**
	 * Maximum size of pattern for HEX memory find functions.
	 */
	static constexpr size_t MAX_PATTERN = 128;

	/**
	 * Converts HEX string containing pairs of symbols 0-9, A-F, a-f with possible space splitting into byte array.
	 */
	static size_t ConvertHexString(const char *srcHexString, unsigned char *outBuffer, size_t bufferSize);

	/**
	 * Finds a sequence of bytes in specified memory region.
	 * @param	pattern		Array of raw bytes to be found.
	 * @param	mask		Bitmask (optional).
	 * @return	Pointer to found sequence or 0.
	 */
	static uintptr_t MemoryFindForward(uintptr_t start, uintptr_t end, const uint8_t *pattern, const uint8_t *mask, size_t pattern_len);

	/**
	 * Finds a sequence of bytes in specified memory region.
	 * @param	pattern		HEX string of bytes to be found.
	 * @param	mask		HEX string of bitmask (optional).
	 * @return	Pointer to found sequence or 0.
	 */
	static uintptr_t MemoryFindForward(uintptr_t start, uintptr_t end, const char *pattern, const char *mask);

	/**
	 * Finds a sequence of bytes in specified memory region, starting from end.
	 * @param	pattern		Array of raw bytes to be found.
	 * @param	mask		Bitmask (optional).
	 * @return	Pointer to found sequence or 0.
	 */
	static uintptr_t MemoryFindBackward(uintptr_t start, uintptr_t end, const uint8_t *pattern, const uint8_t *mask, size_t pattern_len);

	/**
	 * Finds a sequence of bytes in specified memory region, starting from end.
	 * @param	pattern		HEX string of bytes to be found.
	 * @param	mask		HEX string of bitmask (optional).
	 * @return	Pointer to found sequence or 0.
	 */
	static uintptr_t MemoryFindBackward(uintptr_t start, uintptr_t end, const char *pattern, const char *mask);

protected:
	bool m_bIsSDLEngine = false;
	bool m_bLatePatchesDone = false;
	EngineMsgBuf m_MsgBuf;
	SvcHandler *m_pSvcArray = nullptr;
	UserMessage **m_pUserMsgList = nullptr;

	/**
	 * Called from Init() before all other patches for
	 * platform-specific patches.
	 */
	virtual void PlatformPatchesInit();

	/**
	 * Called from Shutdown() after all other patches to unpatch
	 * platform-specific patches.
	 */
	virtual void PlatformPatchesShutdown();

	/**
	 * Called first time RunFrame is called to perform patches that require
	 * fully initialized engine.
	 */
	virtual void PlatformLatePatching();

	/**
	 * Platform-specific
	 * Finds pointers to engine message buffer, its size and position.
	 * Puts them int m_MsgBuf.
	 */
	virtual void FindMsgBuf();

	/**
	 * Platform-specific
	 * Finds pointer to const SVC message array and puts it in m_pSvcArray.
	 */
	virtual void FindSvcArray();

	/**
	 * Platform-specific
	 * Finds user message list and puts first element in m_pUserMsgList.
	 */
	virtual void FindUserMessageList();
};

inline bool CEnginePatches::IsSDLEngine() { return m_bIsSDLEngine; }

inline const CEnginePatches::EngineMsgBuf &CEnginePatches::GetMsgBuf()
{
	return m_MsgBuf;
}

inline SvcHandler *CEnginePatches::GetSvcArray() { return m_pSvcArray; }

inline UserMessage *CEnginePatches::GetUserMsgList()
{
	return (m_pUserMsgList ? *m_pUserMsgList : nullptr);
}

#endif
