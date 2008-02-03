// 
// AYA version 5
//
// �^�C�}�[�N���X CTimer
// written by fifthmoon. 2005
// 

#ifndef	TIMERH
#define	TIMERH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#if defined(POSIX)
# include <sys/time.h>
#endif

//----

namespace yaya {

	class	timer {
		public:
			timer() { start_time_ = get_now_time(); }

			void restart() { start_time_ = get_now_time(); }
			int  elapsed() { return get_now_time() - start_time_; }
			static int  get_now_time() {
#if defined(WIN32) || defined(_WIN32_WCE)
				return ::GetTickCount();
#elif defined(POSIX)
				struct timeval tv;
				gettimeofday(&tv, NULL);
				return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
			}

		private:
			int start_time_;
	};

}; // namespace yaya {

//----

#endif
