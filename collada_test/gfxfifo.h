#ifndef GFXFIFO_H
#define GFXFIFO_H

#include <cassert>
#include <vector>
#include <climits>

namespace gfx_fifo
{
	typedef unsigned int cmd;
	typedef cmd cmd_param;
	
	enum begin_mode
	{
		TRIANGLES      = 0,
		QUADS          = 1,
		TRIANGLE_STRIP = 2,
		QUAD_STRIP     = 3,
		BEGIN_MODE_COUNT
	};

	inline cmd_param packBeginParam(enum begin_mode mode)
	{
		assert(
			(int(mode) >= 0) &&
			(int(mode) < BEGIN_MODE_COUNT)
		);
		return mode;
	}

	inline cmd packColorParam(int r, int g, int b)
	{
		assert(r > 0 && r < (1 << 5));
		assert(g > 0 && g < (1 << 5));
		assert(b > 0 && b < (1 << 5));
		return (r << 10) | (g << 5) | b;
	}

	enum cmd_type
	{
		NOP = 0x0,
		
		// vertex attributes
		COLOR = 0x20,
		NORMAL = 0x21,
		TEXCOORD = 0x22,
		POSITION_16 = 0x23,
		POSITION_10 = 0x24,
		POSITION_XY = 0x25,
		POSITION_XZ = 0x26,
		POSITION_YZ = 0x27,
		POSITION_DIFF = 0x28,
		
		// begin / end
		BEGIN = 0x40,
		END   = 0x41,
	};
	
	inline cmd packCmds(enum cmd_type c1, enum cmd_type c2, enum cmd_type c3, enum cmd_type c4)
	{
		return (int(c4) << 24) | (int(c3) << 16) | (int(c2) << 8) | int(c1);
	}
	
	class GfxFifoStream
	{
	public:
		GfxFifoStream() : cmd_count(0), cmd_param_count(0) {}

		void begin(enum begin_mode mode)
		{
			addParam(packBeginParam(mode));
			addCmd(BEGIN);
		}
		
		void end()
		{
			addCmd(END);
		}
		
		void position16(int x, int y, int z)
		{
			const int min_val = -(1 << 15);
			const int max_val = ((1 << 15) - 1);
			assert(x >= min_val && x <= max_val);
			assert(y >= min_val && y <= max_val);
			assert(z >= min_val && z <= max_val);
			
			addParam((y << 16) | (x & 0xFFFF));
			addParam(z & 0xFFFF);
			addCmd(POSITION_16);
		}
		
		void normal(int x, int y, int z)
		{
			// range-check (debug)
			const int min_val = -(1 << 9);
			const int max_val = ((1 << 9) - 1);
			assert(x >= min_val && x <= max_val);
			assert(y >= min_val && y <= max_val);
			assert(z >= min_val && z <= max_val);

			// remove sign-bits
			const int normal_mask = (1 << 10) - 1;
			x &= normal_mask;
			y &= normal_mask;
			z &= normal_mask;
			
			addParam( (z << 20) | (y << 10) | x );
			addCmd(NORMAL);
		}
		
		void color(int r, int g, int b)
		{
			// range-check (debug)
			assert((r >= 0) && (r < (1 << 5)));
			assert((g >= 0) && (g < (1 << 5)));
			assert((b >= 0) && (b < (1 << 5)));
			
			addParam( (r << 10) | (g << 5) | b );
			addCmd(COLOR);
		}
		
		void texCoord(int u, int v)
		{
			assert(u > -(1 << 15));
			assert(u < ((1 << 15) - 1));
			
			assert(v > -(1 << 15));
			assert(v < ((1 << 15) - 1));
			
			addParam((u & 0xFFFF) | ((v & 0xFFFF) << 16));
			addCmd(TEXCOORD);
		}
		
		std::vector<unsigned int> finalize()
		{
			// pad with NOPs
			for (;cmd_count < 4; ++cmd_count)
			{
				cmds[cmd_count] = NOP;
			}
			
			flush();
			
			return outputBuffer;
		}
		
	private:
		void addCmd(cmd_type cmd)
		{
			assert(cmd_count < 4);
			
			cmds[cmd_count] = cmd;
			cmd_count++;
			if (cmd_count == 4) flush();
		}
		
		void addParam(cmd_param param)
		{
			assert(cmd_param_count < 32 * 4);
			
			cmd_params[cmd_param_count] = param;
			cmd_param_count++;
		}
		
		void flush()
		{
			// write header
			outputBuffer.push_back(packCmds(cmds[0], cmds[1], cmds[2], cmds[3]));
			
			// write params
			for (int i = 0; i < cmd_param_count; ++i)
			{
				outputBuffer.push_back(cmd_params[i]);
			}
			
			// reset counters
			cmd_count = 0;
			cmd_param_count = 0;
		}
		
	private:
		// keeps track of the previous 4 commands (4 and 4 commands are packed together)
		cmd_type cmds[4];
		int cmd_count;
		
		cmd_param cmd_params[32 * 4];
		int cmd_param_count;
		
		std::vector<unsigned int> outputBuffer;
	};
}

#endif /* GFXFIFO_H */
