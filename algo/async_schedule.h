#ifndef ASYNC_SCHEDULE_HEADER
#define ASYNC_SCHEDULE_HEADER

/***************************************************************************
 *            async_schedule.h
 *
 *  Fri Oct 25 16:08:06 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../io/io.h"
#include "../common/utils.h"
#include <queue>
#include <algorithm>

__STXXL_BEGIN_NAMESPACE

struct sim_event // only one type of event: WRITE COMPLETED
{
	int timestamp;
	int iblock;
	sim_event(int t, int b):timestamp(t),iblock(b) {};
};

struct sim_event_cmp
{
	bool operator () (const sim_event & a, const sim_event & b) const
	{
		return a.timestamp > b.timestamp;
	}
};

int simulate_async_write(
	int *disks,
	const int L,
	const int m_init,
	const int D,
	std::pair<int,int> * o_time)
{
	typedef std::priority_queue<sim_event,std::vector<sim_event>,sim_event_cmp> event_queue_type;
	typedef std::queue<int> disk_queue_type;
	
	disk_queue_type * disk_queues = new disk_queue_type[L];
	event_queue_type event_queue;
	
	int m = m_init;
	int i = L - 1;
	int oldtime = 0;
	bool * disk_busy = new bool [D];

	while(m && (i>=0))
	{
		int disk = disks[i];
		disk_queues[disk].push(i);
		i--;
		m--;
	}
	
	for(int i=0;i<D;i++)
		if(!disk_queues[i].empty())
		{
			int j = disk_queues[i].front();
			disk_queues[i].pop();
			event_queue.push(sim_event(1,j));
//			STXXL_MSG("Block "<<j<<" scheduled")
		}
	
	while(! event_queue.empty())
	{
		sim_event cur = event_queue.top();
		event_queue.pop();
		if(oldtime != cur.timestamp)
		{
			// clear disk_busy
			for(int i=0;i<D;i++)
				disk_busy[i] = false;
			oldtime = cur.timestamp;
		}
			
		
		STXXL_MSG("Block "<< cur.iblock <<" put out, time " << cur.timestamp<<" disk: "<<disks[cur.iblock])
		o_time[cur.iblock] = std::pair<int,int>(cur.iblock,cur.timestamp);
		
		m++;
		if(i >= 0)
		{
			m--;
			int disk = disks[i];
			if(disk_busy[disk])
			{
				disk_queues[disk].push(i);
			}
			else
			{
//				STXXL_MSG("Block "<<i<<" scheduled for time "<< cur.timestamp + 1)
				event_queue.push(sim_event(cur.timestamp + 1, i));
				disk_busy[disk] = true;
			}	
			
			i--;
		}
		
		// add next block to write
		int disk = disks[cur.iblock];
		if(!disk_busy[disk] && !disk_queues[disk].empty())
		{
//			STXXL_MSG("Block "<<disk_queues[disk].front()<<" scheduled for time "<< cur.timestamp + 1)
			event_queue.push(sim_event(cur.timestamp + 1,disk_queues[disk].front()));
			disk_queues[disk].pop();
			disk_busy[disk] = true;
		}
		
	};
	
	assert(m == m_init );
	assert(i == -1);
	for(int i=0;i<D;i++)
		assert(disk_queues[i].empty());
	
	delete [] disk_busy;
	delete [] disk_queues;

	return (oldtime - 1);
}


struct write_time_cmp
{
	bool operator () (const std::pair<int,int> & a, const std::pair<int,int> & b)
	{
		return a.second > b.second;
	}
};

void compute_prefetch_schedule(
		int * first,
		int * last,
		int * out_first,
		int m,
		int D)
{
	typedef std::pair<int,int>  pair_type;
	int L = last - first;
	pair_type * write_order = new pair_type[L];
	
	int w_steps=simulate_async_write(first,L,m,D,write_order);
	
	STXXL_MSG("Write steps: " << w_steps )
	
	for(int i=0;i<L;i++)
		STXXL_MSG(first[i] << " " << write_order[i].first << " "<< write_order[i].second )

	std::stable_sort(write_order,write_order + L,write_time_cmp());
	
	for(int i=0;i<L;i++)
	{
		out_first[i] = write_order[i].first;
		//if(out_first[i] != i)
			STXXL_MSG(i << " "<< out_first[i])
	}
	
	delete [] write_order;
}

template <typename run_type>
void simulate_async_write(
											const run_type & input,
											const int m_init,
											const int D,
											std::pair<int,int> * o_time)
{
	typedef std::priority_queue<sim_event,std::vector<sim_event>,sim_event_cmp> event_queue_type;
	typedef std::queue<int> disk_queue_type;
	
	const int L = input.size();
	disk_queue_type * disk_queues = new disk_queue_type[L];
	event_queue_type event_queue;
	
	int m = m_init;
	int i = L - 1;
        int oldtime = 0;
        bool * disk_busy = new bool [D];
		       
	while(m && (i>=0))
	{
		int disk = input[i].bid.storage->get_disk_number();
		disk_queues[disk].push(i);
		i--;
		m--;
	}
	
	for(int i=0;i<D;i++)
		if(!disk_queues[i].empty())
		{
			int j = disk_queues[i].front();
			disk_queues[i].pop();
			event_queue.push(sim_event(1,j));
		}
	
	while(! event_queue.empty())
	{
		sim_event cur = event_queue.top();
		event_queue.pop();
		if(oldtime != cur.timestamp)
    {
			// clear disk_busy
			for(int i=0;i<D;i++)
				disk_busy[i] = false;
			
			oldtime = cur.timestamp;
    }
		o_time[cur.iblock] = std::pair<int,int>(cur.iblock,cur.timestamp + 1);
		
		m++;
		if(i >= 0)
		{
			m--;
			int disk = input[i].bid.storage->get_disk_number();
			if(disk_busy[disk])
			{
				disk_queues[disk].push(i);
			}
			else
			{
				event_queue.push(sim_event(cur.timestamp + 1, i));
				disk_busy[disk] = true;
			}	
			
			i--;
		}
		
		// add next block to write
		int disk = input[cur.iblock].bid.storage->get_disk_number();
		if(!disk_busy[disk] && !disk_queues[disk].empty())
		{
			event_queue.push(sim_event(cur.timestamp + 1,disk_queues[disk].front()));
			disk_queues[disk].pop();
			disk_busy[disk] = true;
		}
		
	};
	
	delete [] disk_busy;
	delete [] disk_queues;
}


template <typename run_type>
void compute_prefetch_schedule(
		const run_type & input,
		int * out_first,
		int m,
		int D)
{
	typedef std::pair<int,int>  pair_type;
	const int L = input.size();
	pair_type * write_order = new pair_type[L];
	
	simulate_async_write(input,m,D,write_order);
	
	std::stable_sort(write_order,write_order + L,write_time_cmp());
	
	for(int i=0;i<L;i++)
		out_first[i] = write_order[i].first;

	delete [] write_order;
}


template <typename bid_iterator_type>
void simulate_async_write(
											bid_iterator_type input,
											const int L,
											const int m_init,
											const int D,
											std::pair<int,int> * o_time)
{
	typedef std::priority_queue<sim_event,std::vector<sim_event>,sim_event_cmp> event_queue_type;
	typedef std::queue<int> disk_queue_type;
	
	disk_queue_type * disk_queues = new disk_queue_type[L];
	event_queue_type event_queue;
	
	int m = m_init;
	int i = L - 1;
        int oldtime = 0;
        bool * disk_busy = new bool [D];
		       
	while(m && (i>=0))
	{
		int disk = (*(input + i)).storage->get_disk_number();
		disk_queues[disk].push(i);
		i--;
		m--;
	}
	
	for(int i=0;i<D;i++)
		if(!disk_queues[i].empty())
		{
			int j = disk_queues[i].front();
			disk_queues[i].pop();
			event_queue.push(sim_event(1,j));
		}
	
	while(! event_queue.empty())
	{
		sim_event cur = event_queue.top();
		event_queue.pop();
		if(oldtime != cur.timestamp)
    {
			// clear disk_busy
			for(int i=0;i<D;i++)
				disk_busy[i] = false;
			
			oldtime = cur.timestamp;
    }
		o_time[cur.iblock] = std::pair<int,int>(cur.iblock,cur.timestamp + 1);
		
		m++;
		if(i >= 0)
		{
			m--;
			int disk = (*(input + i)).storage->get_disk_number();
			if(disk_busy[disk])
			{
				disk_queues[disk].push(i);
			}
			else
			{
				event_queue.push(sim_event(cur.timestamp + 1, i));
				disk_busy[disk] = true;
			}	
			
			i--;
		}
		
		// add next block to write
		int disk = (*(input + cur.iblock)).storage->get_disk_number();
		if(!disk_busy[disk] && !disk_queues[disk].empty())
		{
			event_queue.push(sim_event(cur.timestamp + 1,disk_queues[disk].front()));
			disk_queues[disk].pop();
			disk_busy[disk] = true;
		}
		
	};
	
	delete [] disk_busy;
	delete [] disk_queues;
}


template <typename bid_iterator_type>
void compute_prefetch_schedule(
		bid_iterator_type input_begin,
		bid_iterator_type input_end,
		int * out_first,
		int m,
		int D)
{
	typedef std::pair<int,int>  pair_type;
	const int L = input_end - input_begin;
	pair_type * write_order = new pair_type[L];
	
	simulate_async_write(input_begin,L,m,D,write_order);
	
	std::stable_sort(write_order,write_order + L,write_time_cmp());
	
	for(int i=0;i<L;i++)
		out_first[i] = write_order[i].first;

	delete [] write_order;
}


__STXXL_END_NAMESPACE

#endif
