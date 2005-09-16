
#include "iostats.h"

__STXXL_BEGIN_NAMESPACE

		double wait_time_counter = 0.0;
		stats * stats::instance = NULL;

		stats::stats ():
			reads (0),
			writes (0),
			volume_read(0),
			volume_written(0),
			t_reads (0.0),
			t_writes (0.0),
			p_reads (0.0),
			p_writes (0.0),
			p_begin_read (0.0),
			p_begin_write (0.0),
			p_ios (0.0),
			p_begin_io (0),
			acc_ios (0), acc_reads (0), acc_writes (0),
			last_reset(stxxl_timestamp())
		{
		}
	
		stats * stats::get_instance ()
		{
			if (!instance)
				instance = new stats ();
			return instance;
		}
		
		unsigned stats::get_reads () const
		{
			return reads;
		}
		
		unsigned stats::get_writes () const
		{
			return writes;
		}
		
		int64 stats::get_read_volume () const
		{
			return volume_read;
		}
		
		int64 stats::get_written_volume () const
		{
			return volume_written;
		}
		
		double stats::get_read_time () const
		{
			return t_reads;
		}

		double stats::get_write_time () const
		{
			return t_writes;
		}
		
		double stats::get_pread_time() const
		{
			return p_reads;
		}
		
		double stats::get_pwrite_time() const
		{
			return p_writes;
		}
		
		double stats::get_pio_time() const
		{
			return p_ios;
		}
		
		double stats::get_last_reset_time() const
		{
			return last_reset;
		}
		
		void stats::reset()
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock ReadLock(read_mutex);
			#else
			read_mutex.lock ();
			#endif
			//      assert(acc_reads == 0);
			if (acc_reads)
				STXXL_ERRMSG( "Warning: " << acc_reads <<
					" read(s) not yet finished")

			reads = 0;
			volume_read = 0;
			t_reads = 0;
			p_reads = 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			ReadLock.unlock();
			boost::mutex::scoped_lock WriteLock(write_mutex);
			#else
			read_mutex.unlock ();
			write_mutex.lock ();
			#endif
			
			//      assert(acc_writes == 0);
			if (acc_writes)
				STXXL_ERRMSG("Warning: " << acc_writes <<
					" write(s) not yet finished")

			writes = 0;
			volume_written = 0;
			t_writes = 0.0;
			p_writes = 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			WriteLock.unlock();
			#else
			write_mutex.unlock ();
			#endif
			
			last_reset = stxxl_timestamp();

			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock IOLock(io_mutex);
			#else
			io_mutex.lock ();
			#endif
			
			//      assert(acc_ios == 0);
			if (acc_ios)
				STXXL_ERRMSG( "Warning: " << acc_ios <<
					" io(s) not yet finished" )

			p_ios = 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			IOLock.unlock();
			#else
			io_mutex.unlock ();
			#endif

#ifdef COUNT_WAIT_TIME
			stxxl::wait_time_counter = 0.0;
#endif
			
		}
		
		
#ifdef COUNT_WAIT_TIME
		
		void stats::_reset_io_wait_time() { stxxl::wait_time_counter = 0.0; }
		
		double stats::get_io_wait_time() const { return (stxxl::wait_time_counter); }
		
		double stats::increment_io_wait_time(double val) { return stxxl::wait_time_counter+= val; }
#else
		
		void stats::_reset_io_wait_time() {}
		double stats::get_io_wait_time() const { return -1.0; }
		double stats::increment_io_wait_time(double val) { return -1.0; }
#endif
		
		void stats::write_started (unsigned size_)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock WriteLock(write_mutex);
			#else
			write_mutex.lock ();
			#endif
			double now = stxxl_timestamp ();
			++writes;
			volume_written += size_;
			double diff = now - p_begin_write;
			t_writes += double (acc_writes) * diff;
			p_begin_write = now;
			p_writes += (acc_writes++) ? diff : 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			WriteLock.unlock();
			boost::mutex::scoped_lock IOLock(io_mutex);
			#else
			write_mutex.unlock ();
			io_mutex.lock ();
			#endif
			
			diff = now - p_begin_io;
			p_ios += (acc_ios++) ? diff : 0.0;
			p_begin_io = now;
			
			#ifndef STXXL_BOOST_THREADS
			io_mutex.unlock ();
			#endif
		}
		void stats::write_finished ()
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock WriteLock(write_mutex);
			#else
			write_mutex.lock ();
			#endif
			
			double now = stxxl_timestamp ();
			double diff = now - p_begin_write;
			t_writes += double (acc_writes) * diff;
			p_begin_write = now;
			p_writes += (acc_writes--) ? diff : 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			WriteLock.unlock();
			boost::mutex::scoped_lock IOLock(io_mutex);
			#else
			write_mutex.unlock ();
			io_mutex.lock ();
			#endif
			
			diff = now - p_begin_io;
			p_ios += (acc_ios--) ? diff : 0.0;
			p_begin_io = now;
			
			#ifndef STXXL_BOOST_THREADS
			io_mutex.unlock ();
			#endif
		}
		void stats::read_started (unsigned size_)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock ReadLock(read_mutex);
			#else
			read_mutex.lock ();
			#endif
			double now = stxxl_timestamp ();
			++reads;
			volume_read += size_;
			double diff = now - p_begin_read;
			t_reads += double (acc_reads) * diff;
			p_begin_read = now;
			p_reads += (acc_reads++) ? diff : 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			ReadLock.unlock();
			boost::mutex::scoped_lock IOLock(io_mutex);
			#else
			read_mutex.unlock ();
			io_mutex.lock ();
			#endif
			
			diff = now - p_begin_io;
			p_ios += (acc_ios++) ? diff : 0.0;
			p_begin_io = now;
			
			#ifndef STXXL_BOOST_THREADS
			io_mutex.unlock ();
			#endif
		}
		void stats::read_finished ()
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock ReadLock(read_mutex);
			#else
			read_mutex.lock ();
			#endif
			
			double now = stxxl_timestamp ();
			double diff = now - p_begin_read;
			t_reads += double (acc_reads) * diff;
			p_begin_read = now;
			p_reads += (acc_reads--) ? diff : 0.0;
			
			#ifdef STXXL_BOOST_THREADS
			ReadLock.unlock();
			boost::mutex::scoped_lock IOLock(io_mutex);
			#else
			read_mutex.unlock ();
			io_mutex.lock ();
			#endif
			
			diff = now - p_begin_io;
			p_ios += (acc_ios--) ? diff : 0.0;
			p_begin_io = now;
			
			#ifndef STXXL_BOOST_THREADS
			io_mutex.unlock ();
			#endif
		}
	
	
	#ifndef DISKQUEUE_HEADER
	
	std::ostream & operator << (std::ostream & o, const stats & s)
	{
		o<< "STXXL I/O statistics"<<std::endl;
		o<< " total number of reads                      : "<< s.get_reads() << std::endl;
		o<< " number of bytes read from disks            : "<< s.get_read_volume() << std::endl;
		o<< " time spent in serving all read requests    : "<< s.get_read_time() << " sec."<<std::endl;
		o<< " time spent in reading (parallel read time) : "<< s.get_pread_time() << " sec."<< std::endl;
		o<< " total number of writes                     : "<< s.get_writes() << std::endl;
		o<< " number of bytes written to disks           : "<< s.get_written_volume() << std::endl;
		o<< " time spent in serving all write requests   : "<< s.get_write_time() << " sec."<< std::endl;
		o<< " time spent in writing (parallel write time): "<< s.get_pwrite_time() << " sec."<< std::endl;
		o<< " time spent in I/O (parallel I/O time)      : "<< s.get_pio_time() << " sec."<< std::endl;
		o<< " I/O wait time                              : "<< s.get_io_wait_time() << " sec."<< std::endl;
		o<< " Time since the last reset                  : "<< (stxxl_timestamp()-s.get_last_reset_time()) << " sec."<< std::endl;
		return o;
	}
	
	#endif
	

__STXXL_END_NAMESPACE
