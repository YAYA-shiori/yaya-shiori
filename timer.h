// 
// AYA version 5
//
// タイマークラス CTimer
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
# include <time.h>
#endif

//----

namespace yaya {

	class	timer {
		public:
			timer() { start_time_ = get_now_time(); }

			void restart() { start_time_ = get_now_time(); }
			// GetTickCount と同じく一周しても差が正しくなるよう、符号なしで引く
			int  elapsed() { return static_cast<int>(static_cast<unsigned int>(get_now_time()) - static_cast<unsigned int>(start_time_)); }
			static int  get_now_time() {
#if defined(WIN32) || defined(_WIN32_WCE)
				return ::GetTickCount();
#elif defined(POSIX)
				// 時刻の調整で戻らないよう CLOCK_MONOTONIC を使う
				struct timespec ts;
				if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
					return static_cast<int>(static_cast<unsigned int>(ts.tv_sec) * 1000U + static_cast<unsigned int>(ts.tv_nsec / 1000000));
				}
				struct timeval tv;
				gettimeofday(&tv, NULL);
				return static_cast<int>(static_cast<unsigned int>(tv.tv_sec) * 1000U + static_cast<unsigned int>(tv.tv_usec / 1000));
#endif
			}

		private:
			int start_time_;
	};

}; // namespace yaya {

//----

#endif
