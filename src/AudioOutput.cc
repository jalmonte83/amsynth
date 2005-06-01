/* amSynth
 * (c) 2001-2004 Nick Dowell
 **/

#include "AudioOutput.h"
#include "VoiceAllocationUnit.h"

AudioOutput::AudioOutput()
{
	running = 0;
	recording = 0;
	wavoutfile = "/tmp/amSynth.wav";
	buffer = new float[BUF_SIZE*4];
}

AudioOutput::~AudioOutput()
{
	out.close();
	delete[] buffer;
}

int
AudioOutput::init	( Config & config )
{
	this->config = &config;
	channels = config.channels;
	
#ifdef with_sndfile
	sf_info.samplerate = config.sample_rate;
	sf_info.channels = config.channels;
	
#ifdef SNDFILE_1
	// libsndfile version 1.x:
	sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
#else
	// libsndfile versions < 1.0:
	sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM;
	sf_info.pcmbitwidth = 16;
#endif
#endif
	return 0;
}

void
AudioOutput::startRecording()
{
#ifdef with_sndfile
#ifdef SNDFILE_1
	// libsndfile version 1.x:
	sndfile = sf_open( wavoutfile.c_str(), SFM_WRITE, &sf_info );

	sf_command( sndfile, SFC_SET_NORM_FLOAT, NULL, SF_TRUE );
#else
	// libsndfile versions < 1.0:
	sndfile = sf_open_write( wavoutfile.c_str(), &sf_info );
	sf_command( sndfile, "norm float", (void*)"on", 0 );
#endif
	recording = 1;
#endif
}

void
AudioOutput::stopRecording()
{
	recording = 0;
#ifdef with_sndfile
	sf_close( sndfile );
#endif
}

bool
AudioOutput::Start ()
{
	fprintf (stderr, "AudioOutput::Start ()\n");
	
	if (out.open (*config) == -1) return false;
	out.setRealtime();
	if (0 != PThread::Run ())
	{
		out.close ();
		return false;
	}
	return true;
}

void
AudioOutput::Stop ()
{
	PThread::Stop ();
	PThread::Join ();
	out.close ();
}

extern void sched_realtime ();

void 
AudioOutput::ThreadAction	()
{
	sched_realtime ();
	//struct sched_param sched; sched.sched_priority = 50;
	//sched_setscheduler (0, SCHED_FIFO, &sched);
	//sched_getparam (0, &sched);
	
	while (!ShouldStop ())
	{
		mInput->Process (buffer+128, buffer+192, BUF_SIZE);
		for (int i=0; i<BUF_SIZE; i++)
		{
			buffer[2*i] = buffer[128+i];
			buffer[2*i+1] = buffer[192+i];
		}
#ifdef with_sndfile
		if( recording )
			sf_writef_float( sndfile, buffer, BUF_SIZE );
#endif
		if( out.write( buffer, BUF_SIZE*channels ) == -1 )
			running = 0;
	}
}
