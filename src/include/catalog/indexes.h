#ifndef MINISQL_INDEXES_H
#define MINISQL_INDEXES_H

// #include <_types/_uint32_t.h>
#include <cmath>
#include <memory>

#include "catalog/table.h"
#include "common/rowid.h"
#include "index/generic_key.h"
#include "index/b_plus_tree_index.h"
#include "record/schema.h"
#include "record/type_id.h"

class IndexMetadata {
  friend class IndexInfo;

public:
  static IndexMetadata *Create(const index_id_t index_id, const std::string &index_name,
                               const table_id_t table_id, const std::vector<uint32_t> &key_map,
                               MemHeap *heap);

  uint32_t SerializeTo(char *buf) const;

  uint32_t GetSerializedSize() const;

  static uint32_t DeserializeFrom(char *buf, IndexMetadata *&index_meta, MemHeap *heap);

  inline std::string GetIndexName() const { return index_name_; }

  inline table_id_t GetTableId() const { return table_id_; }

  uint32_t GetIndexColumnCount() const { return key_map_.size(); }

  inline const std::vector<uint32_t> &GetKeyMapping() const { return key_map_; }

  inline index_id_t GetIndexId() const { return index_id_; }

private:
  IndexMetadata() = delete;

  explicit IndexMetadata(const index_id_t index_id, const std::string &index_name,
                         const table_id_t table_id, const std::vector<uint32_t> &key_map) 
                         :index_id_(index_id),
                          index_name_(index_name),
                          table_id_(table_id),
                          key_map_(key_map){}

private:
  static constexpr uint32_t INDEX_METADATA_MAGIC_NUM = 344528;
  index_id_t index_id_;
  std::string index_name_;
  table_id_t table_id_;
  std::vector<uint32_t> key_map_;  /** The mapping of index key to tuple key */
};

/**
 * The IndexInfo class maintains metadata about a index.
 */
class IndexInfo {
public:
  static IndexInfo *Create(MemHeap *heap) {
    void *buf = heap->Allocate(sizeof(IndexInfo));
    return new(buf)IndexInfo();
  }

  ~IndexInfo() {
    delete heap_;
  }

  void Init(IndexMetadata *meta_data, TableInfo *table_info, BufferPoolManager *buffer_pool_manager) {
    // Step1: init index metadata and table info
    // Step2: mapping index key to key schema
    // Step3: call CreateIndex to create the index
    meta_data_=meta_data;
    table_info_=table_info;
    IndexSchema *key_schema=table_info->GetSchema()->ShallowCopySchema(table_info->GetSchema(), meta_data->key_map_, heap_);
    key_schema_=key_schema;
    index_=CreateIndex(buffer_pool_manager);
  }

  inline Index *GetIndex() { return index_; }

  inline std::string GetIndexName() { return meta_data_->GetIndexName(); }

  inline IndexSchema *GetIndexKeySchema() { return key_schema_; }

  inline MemHeap *GetMemHeap() const { return heap_; }

  inline TableInfo *GetTableInfo() const { return table_info_; }

private:
  explicit IndexInfo() : meta_data_{nullptr}, index_{nullptr}, table_info_{nullptr},
                         key_schema_{nullptr}, heap_(new SimpleMemHeap()) {}

  Index *CreateIndex(BufferPoolManager *buffer_pool_manager) {
    uint32_t ofs=sizeof(RowId)+sizeof(size_t);
    size_t n=key_schema_->GetColumnCount();
    ofs+=(n+7)/8;
    for(size_t i=0;i<n;i++){
      TypeId Type=key_schema_->GetColumn(i)->GetType();
      if(Type==kTypeInt)ofs+=sizeof(int32_t);
      else if(Type==kTypeFloat)ofs+=sizeof(float_t);
      else ofs+=sizeof(uint32_t)+key_schema_->GetColumn(i)->GetLength();
    }
    void *buf=nullptr;
    if(ofs<=4){ 
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<4>, RowId, GenericComparator<4>>));
      BPlusTreeIndex<GenericKey<4>, RowId, GenericComparator<4>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<4>, RowId, GenericComparator<4>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;
    }
    if(ofs<=8){ 
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<8>, RowId, GenericComparator<8>>));
      BPlusTreeIndex<GenericKey<8>, RowId, GenericComparator<8>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<8>, RowId, GenericComparator<8>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;
    }    
    if(ofs<=16){ 
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<16>, RowId, GenericComparator<16>>));
      BPlusTreeIndex<GenericKey<16>, RowId, GenericComparator<16>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<16>, RowId, GenericComparator<16>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;
    }
    if(ofs<=32){ 
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<32>, RowId, GenericComparator<32>>));
      BPlusTreeIndex<GenericKey<32>, RowId, GenericComparator<32>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<32>, RowId, GenericComparator<32>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;
    }
    if(ofs<=64){ 
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<64>, RowId, GenericComparator<64>>));
      BPlusTreeIndex<GenericKey<64>, RowId, GenericComparator<64>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<64>, RowId, GenericComparator<64>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;
    }
    if(ofs<=128){
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<128>, RowId, GenericComparator<128>>));
      BPlusTreeIndex<GenericKey<128>, RowId, GenericComparator<128>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<128>, RowId, GenericComparator<128>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;      
    } else {
      buf=heap_->Allocate(sizeof(BPlusTreeIndex<GenericKey<256>, RowId, GenericComparator<256>>));
      BPlusTreeIndex<GenericKey<256>, RowId, GenericComparator<256>> * B_index=
        new(buf)BPlusTreeIndex<GenericKey<256>, RowId, GenericComparator<256>>(meta_data_->index_id_,key_schema_,buffer_pool_manager);
        return B_index;      
    }
  }

private:
  IndexMetadata *meta_data_;
  Index *index_;
  TableInfo *table_info_;
  IndexSchema *key_schema_;
  MemHeap *heap_;
};

#endif //MINISQL_INDEXES_H
