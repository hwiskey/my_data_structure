#include "CompressMap.h"
#include "common.h"
#include "zlib.h"

const int kCompressBlockBufSize = 16*1024;

CompressStorage::CompressStorage( int size /*= 0*/ )
{
	default_bufsize = size > 0 ? size : kCompressBlockBufSize;
}

CompressStorage::~CompressStorage()
{
	Clear();
}


void CompressStorage::_BlockInit( _Block* block )
{
	memset(block, 0, sizeof(_Block));
}


void CompressStorage::_BlockClear( _Block* block )
{
	_BlockClearBuf(block);
	_BlockClearCBuf(block);
	memset(block, 0, sizeof(_Block));
}

void CompressStorage::_BlockClearBuf( _Block* block )
{
	if (block->buf)
		delete [] block->buf;
	block->buf = NULL;
	block->buf_len = 0;
	block->w_off = 0;
}

void CompressStorage::_BlockClearCBuf( _Block* block )
{
	if (block->cbuf)
		delete [] block->cbuf;
	block->cbuf = NULL;
	block->cbuf_len = 0;
}

void CompressStorage::_BlockNew( _Block* block, int size )
{
	memset(block, 0, sizeof(_Block));
	_BlockNewBuf(block, default_bufsize > size ? default_bufsize : size);
}

void CompressStorage::_BlockNewBuf( _Block* block, int size )
{
	if (size > block->buf_len)
	{
		_BlockClearBuf(block);
		block->buf_len = size;
		block->buf = new char [block->buf_len];
	}
	block->w_off = 0;
}

void CompressStorage::_BlockNewCBuf( _Block* block, int size )
{
	if (size > block->cbuf_len)
	{
		_BlockClearCBuf(block);
		block->cbuf_len = size;
		block->cbuf = new char [block->cbuf_len];
	}
}

bool CompressStorage::_BlockIsSufficient( _Block* block, int len )
{
	if (block->cbuf)
		return false;
	if (block->w_off + len > block->buf_len)
		return false;
	return true;
}

bool CompressStorage::_BlockWrite( _Block* block, Pointer* ptr, char* src, int src_len, int w_off /*= -1*/ )
{
	if (w_off == -1)
		w_off = block->w_off;
	memcpy(block->buf + w_off, src, src_len);
	ptr->off = w_off;
	ptr->len = src_len;
	if (w_off == -1)
		block->w_off += src_len;
	else
		block->w_off = max(block->w_off, w_off + src_len);
	return true;
}

bool CompressStorage::_BlockCompress( _Block* block )
{
	block->compress_len = compressBound(block->buf_len);
	std::string buf;
	buf.resize(block->compress_len);
	
	int ret = compress((Bytef *)buf.c_str(), (uLongf *)&block->compress_len, (const Bytef *)block->buf, block->w_off);
	if (ret != Z_OK)
	{
		assert(false);
		return false;
	}
	
	_BlockNewCBuf(block, block->compress_len);
	memcpy(block->cbuf, (char*)buf.c_str(), block->compress_len);
	block->old_len = block->w_off;
	_BlockClearBuf(block);
	return true;
}

bool CompressStorage::_BlockUnCompress( _Block* block )
{
	if (block->cbuf == NULL || block->old_len <= 0 || block->compress_len <= 0)
		return false;
	if (block->buf != NULL)
		return true;
	
	_BlockNewBuf(block, block->old_len);
	int uncompress_len = block->old_len;
	int ret = uncompress((Bytef *)block->buf, (uLongf *)&uncompress_len, (const Bytef *)block->cbuf, block->compress_len);
	if (ret != Z_OK || uncompress_len != block->old_len)
	{
		assert(false);
		return false;
	}

	return true;
}

bool CompressStorage::_BlockCopy( _Block* block, char* dst, int off, int len )
{
	if (off + len > block->cbuf_len)
		return false;

	memcpy(dst, block->cbuf + off, len);
	return true;
}

bool CompressStorage::_BlockMove( _Block* block, _Block* block_src, Pointer* ptr )
{
	int len = ptr->len;
	if (block->w_off + len > block->cbuf_len)
		return false;

	memmove(block->buf + block->w_off, block_src->buf + ptr->off, len);
	ptr->idx = block->idx;
	ptr->off = block->w_off;
	block->w_off += len;
	return true;
}

int CompressStorage::Insert( char* src, int src_len )
{
	// find block
	_Block* block = NULL;
	if (!blocks.empty())
	{
		if (_BlockIsSufficient(&blocks.back(), src_len))
			block = &blocks.back();
	}

	// new block
	if (block == NULL)
	{
		blocks.resize(blocks.size() + 1);
		block = &blocks.back();
		_BlockNew(block, src_len);
		block->idx = (int)blocks.size() - 1;
	}

	// write
	// lazy compress
	Pointer ret;
	ret.idx = block->idx;
	_BlockWrite(block, &ret, src, src_len);

	// write index
	if (keymap.empty())
		ret.key = 1;
	else
		ret.key = keymap.rbegin()->first + 1;
	keymap[ret.key] = ret;

	return ret.key;
}

void CompressStorage::Remove( int key )
{
	//TODO: recycle buf, cbuf
	_PointMap::iterator it = keymap.find(key);
	if (it == keymap.end())
		return;

	keymap.erase(it);
}

CompressStorage::Pointer CompressStorage::Query( int key )
{
	_PointMap::iterator it = keymap.find(key);
	if (it == keymap.end())
	{
		static Pointer null_ptr = {0};
		return null_ptr;
	}

	return it->second;
}

char* CompressStorage::GetData( int key )
{
	Pointer pos = Query(key);
	if (pos.key != key)
		return NULL;

	static int pre_idx = -1;
	if (pre_idx != -1 && pre_idx != pos.idx)
		_BlockClearBuf(&blocks[pre_idx]);
	pre_idx = pos.idx;
	return GetData(pos);
}

char* CompressStorage::GetData( Pointer pos )
{
	if (pos.key <= 0 || pos.idx < 0 || pos.idx >= (int)blocks.size())
		return NULL;

	_Block* block = &blocks[pos.idx];
	_BlockUnCompress(block);

	return block->buf + pos.off;
}

struct pointer_cmp_by_off
{
	typedef CompressStorage::Pointer pointer;
	bool operator() (const pointer* left, const pointer* right) const 
	{
		return left->off < right->off;
	}

	bool operator() (const pointer& left, const pointer& right) const 
	{
		return left.off < right.off;
	}
};

void CompressStorage::Compress()
{
	// Ñ¹Ëõ
	for (_BlockVec::iterator it = blocks.begin(); it != blocks.end(); ++ it)
	{
		_BlockCompress(&(*it));
	}

	// ÄÚ´æÕûÀí
// 	typedef std::vector <Pointer*>		PointerVec;
// 	typedef std::vector <PointerVec>	BlockVec;
// 
// 	BlockVec block_info;
// 	block_info.resize(blocks.size());
// 	for (_PointMap::iterator it = keymap.begin(); it != keymap.end(); ++ it)
// 	{
// 		Pointer& pos = it->second;
// 		block_info[pos.idx].push_back(&pos);
// 	}
// 
// 	int new_idx = 0;
// 	blocks[0].w_off = 0; // re-assign
// 	for (int i = 0; i < (int)block_info.size(); ++ i)
// 	{
// 		std::sort(block_info[i].begin(), block_info[i].end(), pointer_cmp_by_off());
// 		_Block* block = &blocks[new_idx];
// 		_Block* block_old = &blocks[i];
// 
// 		for (PointerVec::iterator it = block_info[i].begin(); it != block_info[i].end(); ++ it)
// 		{
// 			if (!_BlockMove(block, block_old, *it))
// 			{
// 				block = &blocks[++ new_idx];
// 				block->w_off = 0; // re-assign
// 				if (!_BlockMove(block, block_old, *it))
// 					assert(false);
// 			}
// 		}
// 	}
// 
// 	if (blocks[new_idx].w_off == 0)
// 		-- new_idx;
// 
// 	for (int i = new_idx + 1; i < (int)blocks.size(); ++ i)
// 		_BlockClear(&blocks[i]);
// 	blocks.resize(new_idx + 1);
}

int CompressStorage::QueryBytes()
{
	int ret = 0;
	for (_BlockVec::iterator it = blocks.begin(); it != blocks.end(); ++ it)
		ret += it->w_off + it->old_len;
	return ret;
}

int CompressStorage::QueryCBytes()
{
	int ret = 0;
	for (_BlockVec::iterator it = blocks.begin(); it != blocks.end(); ++ it)
		ret += it->compress_len;
	return ret;
}

void CompressStorage::Clear()
{
	keymap.clear();
	for (_BlockVec::iterator it = blocks.begin(); it != blocks.end(); ++ it)
		_BlockClear(&(*it));
	blocks.clear();
}