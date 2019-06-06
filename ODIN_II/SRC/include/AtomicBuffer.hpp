#ifndef SIMULATION_BUFFER_H
#define SIMULATION_BUFFER_H

#include <atomic>
#include <thread>
#include <assert.h>

/**
 * Odin use -1 internally, so we need to go back on forth
 * TODO: change the default odin value to match both the Blif value and this buffer
 */
typedef signed char data_t;

#define BUFFER_SIZE         4   //use something divisible by 4 since the compact buffer can contain 4 value
#define CONCURENCY_LIMIT    (BUFFER_SIZE-1)   // access to cycle -1 with an extra pdding cell

#define BUFFER_ARRAY_SIZE(input_size) (((input_size/4) + ((input_size%4)?1:0)))

class AtomicBuffer
{
private:
	uint8_t bits[BUFFER_ARRAY_SIZE(BUFFER_SIZE)];

    std::atomic_flag lock = ATOMIC_FLAG_INIT;

	int64_t cycle;

    template<typename T>
    uint8_t to_val(T val_in)
    {
        switch(val_in)
        {
            case 0:		//fallthrough
            case '0':	return 0;

            case 1:		//fallthrough
            case '1':	return 1;

            default:	return 2;
        }
    }

    template<typename T>
    data_t val(T val_in)
    {
        switch(val_in)
        {
            case 0:		//fallthrough
            case '0':	return 0;

            case 1:		//fallthrough
            case '1':	return 1;

            default:	return -1;
        }
    }

    template<typename T>
    char val_c(T val_in)
    {
        switch(val_in)
        {
            case 0:		//fallthrough
            case '0':	return '0';

            case 1:		//fallthrough
            case '1':	return '1';

            default:	return 'x';
        }
    }

    void lock_it()
	{
        uint8_t count = 0;
        while(lock.test_and_set(std::memory_order_acquire))
        {
            count = (count + 1) % ((uint8_t)128);
            if(!count)
                std::this_thread::yield();
        }
	}

	void unlock_it()
	{
        lock.clear(std::memory_order_release); 
	}

    template<typename CYCLE_T>
    uint8_t get_addr(CYCLE_T in)
    {
        int64_t cycle_in = static_cast<int64_t>(in) % static_cast<int64_t>(size());
        uint8_t casted_addr = static_cast<uint8_t>(cycle_in);
        return (casted_addr/4);
    }

    template<typename CYCLE_T>
    uint8_t get_index(CYCLE_T in)
    {
        int64_t cycle_in = static_cast<int64_t>(in) % static_cast<int64_t>(size());
        uint8_t casted_addr = static_cast<uint8_t>(cycle_in);
        return (uint8_t)((casted_addr%4) * 2);
    }

    template<typename CYCLE_T>
	data_t get_bits(CYCLE_T index)
	{
        uint8_t addr = get_addr(index);
        uint8_t id = get_index(index);
        uint8_t bitset = bits[addr];

        bitset = (uint8_t)(bitset >> id);
        bitset = (uint8_t)(bitset & 0x3);

        return val(bitset);
	}
	
    template<typename CYCLE_T, typename VAL_T>
	void set_bits(CYCLE_T index, VAL_T value)
	{
        uint8_t addr = get_addr(index);
        uint8_t id = get_index(index);
        uint8_t value_in = to_val(value);

        uint8_t reset_mask = (uint8_t)(0x3);
        reset_mask = (uint8_t)(reset_mask << id);
        reset_mask = (uint8_t)(~reset_mask);

        value_in = (uint8_t)(value_in << id);

        bits[addr] = (uint8_t)(bits[addr] & reset_mask);
        bits[addr] = (uint8_t)(bits[addr] | value_in);
	}

    void lock_free_update_cycle( int64_t cycle_in)
    {
        assert(is_greater_cycle(cycle_in));
        this->cycle = cycle_in;
    }

    bool is_greater_cycle(int64_t cycle_in)
    {
        return (cycle_in >= this->cycle);
    }

public:

    AtomicBuffer(data_t value_in)
    {
        this->cycle = -1;
        for(size_t i=0; i<size(); i++)
            set_bits( i, value_in);
    }

    uint8_t size()
    {
        return (BUFFER_ARRAY_SIZE(BUFFER_SIZE)*4);
    }

    void print()
    {
        for(size_t i=0; i<size(); i++)
            printf("%c",val_c(get_bits(i)));
        
        printf("\n");
    }

    void init_all_values( data_t value_in)
    {
        lock_it();
        for(size_t i=0; i<size(); i++)
            set_bits( i, value_in);
        unlock_it();

    }

    int64_t get_cycle()
    {
		lock_it();
		int64_t value = this->cycle;
        unlock_it();
        return value;
    }

    void update_cycle( int64_t cycle_in)
    {
		lock_it();
        lock_free_update_cycle(cycle_in);
        unlock_it();
    }

    data_t get_value( int64_t cycle_in)
    {
		lock_it();
    	data_t value = get_bits( cycle_in);
        unlock_it();
        return value;
    }

    void update_value( data_t value_in, int64_t cycle_in)
    {
		lock_it();
        lock_free_update_cycle( cycle_in );
        set_bits( cycle_in, value_in );
        unlock_it();
    }
    
};

#endif

