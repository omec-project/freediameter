
#include <string>
#include <unordered_map>

extern "C" {
   int initInt32HashList(void **hl);
   void deleteInt32HashList(void *hl);
   void deleteEntryInt32HashList(int32_t k, void *hl);
   int insertInt32HashList(int32_t k, void *v, void *hl, void **duplicate);
   int findInt32HashList(int32_t k, void *hl, void **result);

   int initInt64HashList(void **hl);
   void deleteInt64HashList(void *hl);
   void deleteEntryInt64HashList(int64_t k, void *hl);
   int insertInt64HashList(int64_t k, void *v, void *hl, void **duplicate);
   int findInt64HashList(int64_t k, void *hl, void **result);

   int initUInt32HashList(void **hl);
   void deleteUInt32HashList(void *hl);
   void deleteEntryUInt32HashList(uint32_t k, void *hl);
   int insertUInt32HashList(uint32_t k, void *v, void *hl, void **duplicate);
   int findUInt32HashList(uint32_t k, void *hl, void **result);

   int initUInt64HashList(void **hl);
   void deleteUInt64HashList(void *hl);
   void deleteEntryUInt64HashList(uint64_t k, void *hl);
   int insertUInt64HashList(uint64_t k, void *v, void *hl, void **duplicate);
   int findUInt64HashList(uint64_t k, void *hl, void **result);

   int initFloat32HashList(void **hl);
   void deleteFloat32HashList(void *hl);
   void deleteEntryFloat32HashList(float k, void *hl);
   int insertFloat32HashList(float k, void *v, void *hl, void **duplicate);
   int findFloat32HashList(float k, void *hl, void **result);

   int initFloat64HashList(void **hl);
   void deleteFloat64HashList(void *hl);
   void deleteEntryFloat64HashList(double k, void *hl);
   int insertFloat64HashList(double k, void *v, void *hl, void **duplicate);
   int findFloat64HashList(double k, void *hl, void **result);

   int initStringHashList(void **hl);
   void deleteStringHashList(void *hl);
   void deleteEntryStringHashList(const char *k, void *hl);
   int insertStringHashList(const char *k, void *v, void *hl, void **duplicate);
   int findStringHashList(const char *k, void *hl, void **result);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initInt32HashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<int32_t,void*>());
   return 0;
}

void deleteInt32HashList(void *hl)
{
   delete (std::unordered_map<int32_t,void*>*)hl;
}

void deleteEntryInt32HashList(int32_t k, void *hl)
{
   std::unordered_map<int32_t,void*> &l( *(std::unordered_map<int32_t,void*>*)hl );

   l.erase(k);
}

int insertInt32HashList(int32_t k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<int32_t,void*> &l( *(std::unordered_map<int32_t,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findInt32HashList(int32_t k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<int32_t,void*> &l( *(std::unordered_map<int32_t,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initInt64HashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<int64_t,void*>());
   return 0;
}

void deleteInt64HashList(void *hl)
{
   delete (std::unordered_map<int64_t,void*>*)hl;
}

void deleteEntryInt64HashList(int64_t k, void *hl)
{
   std::unordered_map<int64_t,void*> &l( *(std::unordered_map<int64_t,void*>*)hl );

   l.erase(k);
}

int insertInt64HashList(int64_t k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<int64_t,void*> &l( *(std::unordered_map<int64_t,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findInt64HashList(int64_t k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<int64_t,void*> &l( *(std::unordered_map<int64_t,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initUInt32HashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<uint32_t,void*>());
   return 0;
}

void deleteUInt32HashList(void *hl)
{
   delete (std::unordered_map<uint32_t,void*>*)hl;
}

void deleteEntryUInt32HashList(uint32_t k, void *hl)
{
   std::unordered_map<uint32_t,void*> &l( *(std::unordered_map<uint32_t,void*>*)hl );

   l.erase(k);
}

int insertUInt32HashList(uint32_t k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<uint32_t,void*> &l( *(std::unordered_map<uint32_t,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findUInt32HashList(uint32_t k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<uint32_t,void*> &l( *(std::unordered_map<uint32_t,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initUInt64HashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<uint64_t,void*>());
   return 0;
}

void deleteUInt64HashList(void *hl)
{
   delete (std::unordered_map<uint64_t,void*>*)hl;
}

void deleteEntryUInt64HashList(uint64_t k, void *hl)
{
   std::unordered_map<uint64_t,void*> &l( *(std::unordered_map<uint64_t,void*>*)hl );

   l.erase(k);
}

int insertUInt64HashList(uint64_t k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<uint64_t,void*> &l( *(std::unordered_map<uint64_t,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findUInt64HashList(uint64_t k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<uint64_t,void*> &l( *(std::unordered_map<uint64_t,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initFloat32HashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<float,void*>());
   return 0;
}

void deleteFloat32HashList(void *hl)
{
   delete (std::unordered_map<float,void*>*)hl;
}

void deleteEntryFloat32HashList(float k, void *hl)
{
   std::unordered_map<float,void*> &l( *(std::unordered_map<float,void*>*)hl );

   l.erase(k);
}

int insertFloat32HashList(float k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<float,void*> &l( *(std::unordered_map<float,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findFloat32HashList(float k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<float,void*> &l( *(std::unordered_map<float,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initFloat64HashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<double,void*>());
   return 0;
}

void deleteFloat64HashList(void *hl)
{
   delete (std::unordered_map<double,void*>*)hl;
}

void deleteEntryFloat64HashList(double k, void *hl)
{
   std::unordered_map<double,void*> &l( *(std::unordered_map<double,void*>*)hl );

   l.erase(k);
}

int insertFloat64HashList(double k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<double,void*> &l( *(std::unordered_map<double,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findFloat64HashList(double k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<double,void*> &l( *(std::unordered_map<double,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int initStringHashList(void **hl)
{
   *hl = (void*)(new std::unordered_map<std::string,void*>());
   return 0;
}

void deleteStringHashList(void *hl)
{
   delete (std::unordered_map<std::string,void*>*)hl;
}

void deleteEntryStringHashList(const char *k, void *hl)
{
   std::unordered_map<std::string,void*> &l( *(std::unordered_map<std::string,void*>*)hl );

   l.erase(k);
}

int insertStringHashList(const char *k, void *v, void *hl, void **duplicate)
{
   std::unordered_map<std::string,void*> &l( *(std::unordered_map<std::string,void*>*)hl );

   auto result = l.insert({k,v});

   if (!result.second && duplicate)
      *duplicate = result.first->second;

   return result.second ? 0 : EEXIST;
}

int findStringHashList(const char *k, void *hl, void **result)
{
   if (hl == NULL || result == NULL)
      return EINVAL;

   int ret = 0;
   std::unordered_map<std::string,void*> &l( *(std::unordered_map<std::string,void*>*)hl );

   auto search = l.find(k);

   if (search != l.end())
      *result = search->second;
   else
      ret = ENOENT;

   return ret;
}
