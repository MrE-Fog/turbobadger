// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "tb_hashtable.h"
#include "tb_system.h"

namespace tinkerbell {

//FIX: reduce memory (block allocation of ITEM)
//FIX: should shrink when deleting single items (but not when adding items!)
//FIX: should grow when about 70% full instead of 100%

// == TBHashTable =======================================================================

TBHashTable::TBHashTable()
	: m_buckets(0)
	, m_num_buckets(0)
	, m_num_items(0)
{
}

TBHashTable::~TBHashTable()
{
	RemoveAll();
}

void TBHashTable::RemoveAll(bool delete_content)
{
#ifdef _DEBUG
	Debug();
#endif
	for (uint32 i = 0; i < m_num_buckets; i++)
	{
		ITEM *item = m_buckets[i];
		while (item)
		{
			ITEM *item_next = item->next;
			if (delete_content)
				DeleteContent(item->content);
			delete item;
			item = item_next;
		}
	}
	delete [] m_buckets;
	m_buckets = nullptr;
	m_num_buckets = m_num_items = 0;
}

bool TBHashTable::Rehash(uint32 new_num_buckets)
{
	if (new_num_buckets == m_num_buckets)
		return true;
	if (ITEM **new_buckets = new ITEM*[new_num_buckets])
	{
		memset(new_buckets, 0, sizeof(ITEM*) * new_num_buckets);
		// Rehash all items into the new buckets
		for (uint32 i = 0; i < m_num_buckets; i++)
		{
			ITEM *item = m_buckets[i];
			while (item)
			{
				ITEM *item_next = item->next;
				// Add it to new_buckets
				uint32 bucket = item->key % new_num_buckets;
				item->next = new_buckets[bucket];
				new_buckets[bucket] = item;
				item = item_next;
			}
		}
		// Delete old buckets and update
		delete [] m_buckets;
		m_buckets = new_buckets;
		m_num_buckets = new_num_buckets;
		return true;
	}
	return false;
}

bool TBHashTable::NeedRehash() const
{
	// Grow if more items than buckets
	return !m_num_buckets || m_num_items >= m_num_buckets;
}

uint32 TBHashTable::GetSuitableBucketsCount() const
{
	//const uint32 num_primes = 23;
	//static const uint32 primes[num_primes] = { 29, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613,
	//										393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653, 100663319 };
	//for (uint32 i = 0; i < num_primes; i++)
	//	if (primes[i] > m_num_items)
	//		return primes[i];
	//return 201326611;
	// As long as we use FNV for TBID (in TBGetHash), power of two hash sizes are the best.
	if (!m_num_items)
		return 16;
	return m_num_items * 2;
}

void *TBHashTable::Get(uint32 key) const
{
	if (!m_num_buckets)
		return nullptr;
	uint32 bucket = key % m_num_buckets;
	ITEM *item = m_buckets[bucket];
	while (item)
	{
		if (item->key == key)
			return item->content;
		item = item->next;
	}
	return nullptr;
}

bool TBHashTable::Add(uint32 key, void *content)
{
	if (NeedRehash() && !Rehash(GetSuitableBucketsCount()))
		return false;
	assert(!Get(key));
	if (ITEM *item = new ITEM)
	{
		uint32 bucket = key % m_num_buckets;
		item->key = key;
		item->content = content;
		item->next = m_buckets[bucket];
		m_buckets[bucket] = item;
		m_num_items++;
		return true;
	}
	return false;
}

#ifdef _DEBUG

void TBHashTable::Debug()
{
	TBStr line("Hash table: ");
	int total_count = 0;
	for (uint32 i = 0; i < m_num_buckets; i++)
	{
		int count = 0;
		ITEM *item = m_buckets[i];
		while (item)
		{
			count++;
			item = item->next;
		}
		TBStr tmp; tmp.SetFormatted("%d ", count);
		line.Append(tmp);
		total_count += count;
	}
	TBStr tmp; tmp.SetFormatted(" (total: %d of %d buckets)\n", total_count, m_num_buckets);
	line.Append(tmp);
	TBDebugOut(line);
}

#endif // _DEBUG

// == TBHashTableIterator ===============================================================

TBHashTableIterator::TBHashTableIterator(TBHashTable *hash_table)
	: m_hash_table(hash_table)
	, m_current_bucket(0)
	, m_current_item(nullptr)
{
}

void *TBHashTableIterator::GetNextContent()
{
	if (m_current_bucket == m_hash_table->m_num_buckets)
		return nullptr;
	if (m_current_item && m_current_item->next)
		m_current_item = m_current_item->next;
	else
	{
		if (m_current_item)
			m_current_bucket++;
		if (m_current_bucket == m_hash_table->m_num_buckets)
			return nullptr;
		while (m_current_bucket < m_hash_table->m_num_buckets)
		{
			m_current_item = m_hash_table->m_buckets[m_current_bucket];
			if (m_current_item)
				break;
			m_current_bucket++;
		}
	}
	return m_current_item ? m_current_item->content : nullptr;
}

}; // namespace tinkerbell