//
//		MMMAN : Multimedia manager source
//				for SDL enviroment
//				onion software/onitama 2012/6
//				zakki 2014/7
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "../../hsp3/hsp3config.h"
#include "../../hsp3/dpmread.h"
#include "../../hsp3/strbuf.h"
#include "../supio.h"

#include "mmman.h"

#define sndbank(a) (char *)(mem_snd[a].mempt)

#define MIX_MAX_CHANNEL 128

//---------------------------------------------------------------------------

//	SDL Music Object
Mix_Music *m_music;

void MusicInit( void )
{
	m_music = NULL;
}

void MusicTerm( void )
{
	if (m_music) {
		Mix_HaltMusic();
		Mix_FreeMusic(m_music);
		m_music = NULL;
	}
}

int MusicLoad( char *fname )
{
	MusicTerm();
	m_music=Mix_LoadMUS((const char *)fname);
	if (m_music==NULL) return -1;
	return 0;
}

void MusicPlay( int mode )
{
	if (m_music==NULL) return;
	Mix_HaltMusic();
	Mix_PlayMusic(m_music,mode);
}

void MusicStop( void )
{
	if (m_music==NULL) return;
	Mix_HaltMusic();
}

void MusicPause( void )
{
	if (m_music==NULL) return;
	if (Mix_PausedMusic()!=0) {
		Mix_ResumeMusic();
	} else {
		Mix_PauseMusic();
	}
}

void MusicVolume( int vol )
{
	if (m_music==NULL) return;
	Mix_VolumeMusic(vol);
}

int MusicStatue( int type )
{
	if (m_music==NULL) return 0;
	int res=0;
	switch(type) {
	case 16:
		res = Mix_PlayingMusic();
		break;
	default:
		break;
	}
	return res;
}

//---------------------------------------------------------------------------

//	MMDATA structure
//
typedef struct MMM
{
	//	Multimedia Data structure
	//
	int		flag;			//	bank mode (0=none/1=wav/2=mid/3=cd/4=avi)
	int		opt;			//	option (0=none/1=loop/2=wait/16=fullscr)
	int		num;			//	request number
	short	track;			//	CD track No.
	short	lasttrk;		//	CD last track No.
	void	*mempt;			//	pointer to sound data
	char	*fname;			//	sound filename (sbstr)
	// int		vol;
	// int		pan;
	int		start;
	int		end;
	
	Mix_Chunk	*chunk;
	int		channel;
	// int	pause_flag;

} MMM;


//---------------------------------------------------------------------------

MMMan::MMMan()
{
	mm_cur = 0;

	mem_snd = NULL;
	engine_flag = false;
	MusicInit();

	Mix_Init(MIX_INIT_OGG|MIX_INIT_MP3);
	// Mix_ReserveChannels(16);

	int ret = Mix_OpenAudio(0, 0, 0, 0);
	//assert(ret == 0);

	engine_flag = ret == 0;

}


MMMan::~MMMan()
{
	ClearAllBank();
	MusicTerm();

	while(Mix_Init(0))
		Mix_Quit();
	Mix_CloseAudio();
}

void MMMan::DeleteBank( int bank )
{
	MMM *m;
	char *lpSnd;

	m = &(mem_snd[bank]);
	if ( m->flag == MMDATA_INTWAVE ) {
		StopBank( m );
		Mix_FreeChunk( m->chunk );
	}
	
	if (m->fname!=NULL) {
		free(m->fname);
		m->fname = NULL;
	}
	
	lpSnd = sndbank( bank );
	if ( lpSnd != NULL ) {
		free( lpSnd );
	}
	mem_snd[bank].mempt=NULL;
}


int MMMan::AllocBank( void )
{
	if ( !engine_flag ) return -1;

	int ids = mm_cur++;
	int sz = mm_cur * sizeof(MMM);
	if ( mem_snd == NULL ) {
		mem_snd = (MMM *)sbAlloc( sz );
	} else {
		mem_snd = (MMM *)sbExpand( (char *)mem_snd, sz );
	}
	mem_snd[ids].flag = MMDATA_NONE;
	mem_snd[ids].num = -1;
	mem_snd[ids].channel = -1;
	return ids;
}


int MMMan::SearchBank( int num )
{
	for(int a=0;a<mm_cur;a++) {
		if ( mem_snd[a].num == num ) return a;
	}
	return -1;
}


MMM *MMMan::SetBank( int num, int flag, int opt, void *mempt, char *fname, int start, int end )
{
	int bank;
	MMM *m;

	bank = SearchBank( num );
	if ( bank < 0 ) {
		bank = AllocBank();
	} else {
		DeleteBank( bank );
	}

	if ( bank < 0 ) return NULL;

	m = &(mem_snd[bank]);
	m->flag = flag;
	m->opt = opt;
	m->num = num;
	m->mempt = mempt;
	m->fname = NULL;
	// m->pause_flag = 0;
	// m->vol = 0;
	// m->pan = 0;
	m->start = start;
	m->end = end;
	m->chunk = NULL;
	m->channel = -1;
	return m;
}


void MMMan::ClearAllBank( void )
{
	int a;
	if ( mem_snd != NULL ) {
		Stop();
		for(a=0;a<mm_cur;a++) {
			DeleteBank( a );
		}
		sbFree( mem_snd );
		mem_snd = NULL;
		mm_cur = 0;
	}
}


void MMMan::Reset( void *hwnd )
{
	ClearAllBank();
}


void MMMan::SetWindow( void *hwnd, int x, int y, int sx, int sy )
{
}


void MMMan::Pause( void )
{
	//		pause all playing sounds
	//
	Mix_Pause( -1 );
	MusicPause();
}


void MMMan::Resume( void )
{
	//		resume all playing sounds
	//
	Mix_Resume( -1 );
	MusicPause();
}


void MMMan::Stop( void )
{
	//		stop all playing sounds
	//
	MusicStop();
	Mix_HaltChannel( -1 );
}


void MMMan::StopBank( MMM *mmm )
{
}


void MMMan::PauseBank( MMM *mmm )
{
}


void MMMan::ResumeBank( MMM *mmm )
{
}


void MMMan::PlayBank( MMM *mmm )
{
}


int MMMan::BankLoad( MMM *mmm, char *fname )
{
	char fext[8];
	if ( mmm == NULL ) return -9;

	getpath(fname,fext,16+2);
	if (!strcmp(fext,".mp3")) {
		mmm->fname = (char *)malloc( strlen(fname)+1 );
		strcpy( mmm->fname,fname );
		mmm->flag = MMDATA_MUSIC;
		return 0;
	}

	mmm->chunk = Mix_LoadWAV( fname );
	mmm->channel = mmm->num;
	mmm->flag = MMDATA_INTWAVE;
	return 0;
}


int MMMan::Load( char *fname, int num, int opt, int start, int end )
{
	//		Load sound to bank
	//			opt : 0=normal/1=loop/2=wait/3=continuous
	//
	if ( num < 0 || num >= MIX_MAX_CHANNEL ) return -1;
	int flag,res;
	MMM *mmm;

	flag = MMDATA_INTWAVE;
	mmm = SetBank( num, flag, opt, NULL, fname, start, end );

	if ( mmm != NULL ) {
		res = BankLoad( mmm, fname );
		if ( res ) {
			mmm->flag = MMDATA_NONE;
			Alertf( "[MMMan] Failed %s on bank #%d (%d)",fname,num,res );
			return -1;
		}
		//if ( opt == 1 ) SetLoopBank( mmm, opt );
	}
	Alertf( "[MMMan] Loaded %s on bank #%d",fname,num );
	return 0;
}


int MMMan::Play( int num, int ch )
{
	//		Play sound
	//
	int bank;
	MMM *m;
	bank = SearchBank(num);
	if ( bank < 0 ) return 1;
	m = &(mem_snd[bank]);
	if ( m->flag == MMDATA_INTWAVE && m != NULL ) {
		if ( ch < 0 ) ch = m->channel;
		if ( ch >= 0 && ch < MIX_MAX_CHANNEL ) {
			bool loop = m->opt & 1;
			Mix_HaltChannel( ch );
#ifdef HSPLINUX
			Mix_PlayChannel( ch, m->chunk, loop ? -1 : 0 );
#else
			Mix_PlayChannel( ch, m->chunk, loop ? -1 : 0 , m->start, m->end );
#endif
		}
	}
	return 0;
}


void MMMan::Notify( void )
{
}


void MMMan::GetInfo( int bank, char **fname, int *num, int *flag, int *opt )
{
	//		Get MMM info
	//
	MMM *mmm;
	mmm=&mem_snd[bank];
	*fname = mmm->fname;
	*opt=mmm->opt;
	*flag=mmm->flag;
	*num=mmm->num;
}


void MMMan::SetVol( int num, int vol )
{
	if ( num < 0 || num >= MIX_MAX_CHANNEL ) return;
	if ( vol >     0 ) vol =     0;
	if ( vol < -1000 ) vol = -1000;
	Mix_Volume( num, (int)(MIX_MAX_VOLUME * ((float)(vol + 1000) / 1000.0f)) );
}



void MMMan::SetPan( int num, int pan )
{
	if ( num < 0 || num >= MIX_MAX_CHANNEL ) return;
	if ( pan >  1000 ) pan =  1000;
	if ( pan < -1000 ) pan = -1000;
	int l, r;
	int max = 255;
	if ( pan > 0 ) {
		l = (1000 - pan) * max;
		r = 1000 * max;
	} else {
		l = 1000 * max;
		r = (1000 + pan) * max;
	}
	Mix_SetPanning( num, l / 1000, r / 1000 );
}


int MMMan::GetStatus( int num, int infoid )
{
	if ( num >= MIX_MAX_CHANNEL ) return 0;
	int res = 0;
	switch( infoid ) {
	case 16:
		if ( num < 0 ) {
			res = Mix_Playing( -1 );
		} else {
			res = Mix_Playing( num );
		}
		break;
	}
	return res;
}


void MMMan::StopBank( int num )
{
	//		stop playing sound
	//
	if ( num < 0 ) {
		Stop();
	} else if ( num < MIX_MAX_CHANNEL ) {
		Mix_HaltChannel( num );
	}
	return;
}
