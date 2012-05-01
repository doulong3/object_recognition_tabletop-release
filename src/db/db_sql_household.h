/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef DB_COUCH_H_
#define DB_COUCH_H_

#include <object_recognition_core/db/db.h>
#include <object_recognition_core/db/db_base.h>
#include <object_recognition_core/common/types.h>
#include <household_objects_database/objects_database.h>

using object_recognition_core::db::AttachmentName;
using object_recognition_core::db::CollectionName;
using object_recognition_core::db::DbType;
using object_recognition_core::db::DocumentId;
using object_recognition_core::db::ObjectId;
using object_recognition_core::db::MimeType;
using object_recognition_core::db::ObjectDbParametersRaw;
using object_recognition_core::db::RevisionId;
using object_recognition_core::db::View;
using object_recognition_core::db::ViewElement;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ObjectDbSqlHousehold: public object_recognition_core::db::ObjectDbBase
{
public:
  ObjectDbSqlHousehold();

  ObjectDbSqlHousehold(ObjectDbParametersRaw & parameters);

  ObjectDbParametersRaw
  default_raw_parameters() const;

  virtual void
  insert_object(const or_json::mObject &fields, DocumentId & document_id, RevisionId & revision_id);

  virtual void
  persist_fields(const DocumentId & document_id, const or_json::mObject &fields, RevisionId & revision_id);

  virtual void
  load_fields(const DocumentId & document_id, or_json::mObject &fields);

  virtual void
  get_attachment_stream(const DocumentId & document_id, const std::string& attachment_name,
                        const std::string& content_type, std::ostream& stream, RevisionId & revision_id);

  virtual void
  set_attachment_stream(const DocumentId & document_id, const AttachmentName& attachment_name,
                        const MimeType& mime_type, const std::istream& stream, RevisionId & revision_id);

  virtual
  void
  Delete(const ObjectId & id);

  virtual
  void
  Query(const View & view, int limit_rows, int start_offset, int& total_rows, int& offset,
        std::vector<ViewElement> & view_elements);

  virtual void
  Query(const std::vector<std::string> & queries, int limit_rows, int start_offset, int& total_rows, int& offset,
        std::vector<ViewElement> & view_elements);

  virtual std::string
  Status() const;

  virtual std::string
  Status(const CollectionName& collection) const;

  virtual void
  CreateCollection(const CollectionName &collection);

  virtual void
  DeleteCollection(const CollectionName &collection);

  virtual DbType
  type() const
  {
    return "SqlHousehold";
  }
private:

  inline void
  precondition_id(const DocumentId & id) const
  {
    if (id.empty())
      throw std::runtime_error("The document's id must be initialized.");
  }

  inline void
  precondition_rev(const RevisionId & rev) const
  {
    if (rev.empty())
      throw std::runtime_error("The document must have a valid revision.");
  }

  void
  upload_json(const or_json::mObject &ptree, const std::string& url, const std::string& request);

  template<typename T>
  void
  read_json(T &reader, or_json::mObject& object)
  {
    or_json::mValue value;
    or_json::read(reader, value);
    object = value.get_obj();
  }

  template<typename T>
  void
  write_json(const or_json::mObject& object, T &writer)
  {
    or_json::mValue value(object);
    or_json::write(value, writer);
  }

  void
  GetObjectRevisionId(DocumentId& document_id, RevisionId & revision_id);

  void
  GetRevisionId(RevisionId & revision_id);

  /** Once json_reader_stream_ has been filled, call that function to get the results of the view
   *
   */
  void
  QueryView(const CollectionName & collection_name, int limit_rows, int start_offset, const std::string &options,
            int& total_rows, int& offset, std::vector<ViewElement> & view_elements, bool do_throw);

  boost::shared_ptr<household_objects_database::ObjectsDatabase> db_;
};

#endif /* DB_COUCH_H_ */
