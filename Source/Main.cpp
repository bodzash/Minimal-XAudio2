#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <XAudio2.h>

#include "../External/stb_vorbis.c"

int main(void)
{
	// ---------------------------------------------------------------------------------------------------------

	// XAudio2 related varaibles
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2MasteringVoice* pMasterVoice = nullptr;
	IXAudio2SourceVoice* pSourceVoice = nullptr;

	// Other variables
	bool bShouldRun = true;

	// ---------------------------------------------------------------------------------------------------------

	// Need to init COM, classic Microsoft bullshit, but it makes sense, if you know, you know
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
	{
		printf("[ERROR]: Failed to initialize COM!\n");
		return 1;
	}

	// Create that Audio interface thing
    if (FAILED(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		printf("[ERROR]: Failed to create XAudio2!\n");

        CoUninitialize();
        return 1;
    }

	// Create a MasteringVoice
	if (FAILED(pXAudio2->CreateMasteringVoice(&pMasterVoice)))
	{
		printf("[ERROR]: Failed to create MasteringVoice!\n");

		pXAudio2->Release();
        CoUninitialize();
		return 1;
	}

	// Create a Wave format (Must match the Vorbis file or you'll get some weird shit)
	// NOTE: It's fine to only create a few of these, one for Mono and Stereo
	WAVEFORMATEX WaveFormatDesc = {};
	WaveFormatDesc.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormatDesc.nSamplesPerSec = 44100;
	WaveFormatDesc.wBitsPerSample = 16;
	WaveFormatDesc.nChannels = 2;
	WaveFormatDesc.nBlockAlign = WaveFormatDesc.nChannels * WaveFormatDesc.wBitsPerSample / 8;
	WaveFormatDesc.nAvgBytesPerSec = WaveFormatDesc.nSamplesPerSec * WaveFormatDesc.nBlockAlign;
	WaveFormatDesc.cbSize = 0;

	// Create a SourceVoice (You play audio trough these by feeding them buffer(s))
	// NOTE: In a real game, you want to prealloc a shit-ton of these (64-128)
	if (FAILED(pXAudio2->CreateSourceVoice(&pSourceVoice, &WaveFormatDesc)))
	{
		printf("[ERROR]: Failed to create a SourceVoice!\n");

		pMasterVoice->DestroyVoice();
		pXAudio2->Release();
        CoUninitialize();
		return 1;
	}

	// ---------------------------------------------------------------------------------------------------------

	// Load a Vorbis file and decode it into raw PCM (it's a compressed format)
	// NOTE: In a real game you want raw PCM data for SHORT sounds (less than ~5 sec)
	// NOTE: In a real game you want to STREAM and DECODE sound data for LONG sounds (like music) on a loader thread
    int VorbisChannels = 0;
	int VorbisSampleRate = 0;
    short* VorbisData = nullptr;
    int VorbisSamples = stb_vorbis_decode_filename("./Data/Sample.ogg", &VorbisChannels, &VorbisSampleRate, &VorbisData);
    if (VorbisSamples == -1)
	{
		printf("[ERROR]: Failed to load or decode the Vorbis file!\n");

		pSourceVoice->DestroyVoice();
		pMasterVoice->DestroyVoice();
		pXAudio2->Release();
		CoUninitialize();
        return 1;
    }

	// Point that decoded PCM data into the SourceVoice
	XAUDIO2_BUFFER BufferDesc = {};
	BufferDesc.AudioBytes = VorbisSamples * VorbisChannels * sizeof(short);
	BufferDesc.pAudioData = (BYTE*)VorbisData;
	BufferDesc.Flags = XAUDIO2_END_OF_STREAM;

	pSourceVoice->SubmitSourceBuffer(&BufferDesc);

	// ---------------------------------------------------------------------------------------------------------

	printf("[INFO] Sound Author: https://opengameart.org/users/remaxim\n");
	printf("[INFO] Vorbis Decoder: http://nothings.org\n");
	printf("[GAME]: Press [ Space ] to play the sound!\n");

	// ---------------------------------------------------------------------------------------------------------

	bool PrevFrameKeyState = false;

	while (bShouldRun)
	{		
		// Next frame we exit
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			bShouldRun = false;
		}
	
		// Play the fucking sound already!
		// Iv'e been reading this shit for too long!
		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0 && !PrevFrameKeyState)
		{
			pSourceVoice->Stop(0);
			pSourceVoice->FlushSourceBuffers();
			pSourceVoice->SubmitSourceBuffer(&BufferDesc);
			pSourceVoice->Start(0);
		}
		PrevFrameKeyState = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

		// We don't wanna drain the CPU, so sleep a bit
		Sleep(16);
	}

	// ---------------------------------------------------------------------------------------------------------

	// Don't forget to clean up our own shit
	// It's end of the program anyway so it's not mandatory
	free(VorbisData);
	pSourceVoice->DestroyVoice();
    pMasterVoice->DestroyVoice();
    pXAudio2->Release();
    CoUninitialize();

	return 0;
}