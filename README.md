# librecjar

Library under MIT license for reading/writing record jar formatted text files
as described in "The record-jar Format" in draft
    ["draft-phillips-record-jar-02"]
    (http://tools.ietf.org/html/draft-phillips-record-jar-02 "").

At the moment only US-ASCII character encoding is supported.

## Principles

Every field-value combination in every record is allowed except multiple
fields of the same name in one record. (Technically fields with the same name
as one privious field are just ignored surviving the whole operation and are
saved back with the record into the file.)

The records are stored in a double linked circular list.
A record is identified by a matching field and its value hereinafter called
matching criteria.

After retrieving/creating/setting values the containing record is stored.
At future invocations of these methods this record is searched first.
Retrieving multiple values from one record has therefore complexity O(1) in
means of record traversal.
If the record matching critera are not met the following records are searched.
The search ends if the starting from record is reached again.

Multiple invocations of the mentioned methods with matching critera contained
by multiple records would only operate on the same record. Therefore for every
of these methods next and previous methods are available which skips the
current matched record and proceed with the next/previuous. If some unique
matching criteria is used the normal methods are sufficient.

## Example

An example is included mainly for debugging purposes at the end of the lib.
Therefore an example file named test.rj is also included which is read by
the debugging example.

## Build

following make targets are available
* all: compile into object
* lib: compile into object and pack with ar to static lib
* debug: compile with main and create debug executable

## Methods

### rj_load

* loads a record jar from the specified file into the given jar
* a check whether the character encoding is US-ASCII is performed
* comments are discarded

### rj_save

* saves the given jar into the specified file
* character encoding is US-ASCII

### rj_free

* frees the memory for the given jar
* the recordjar itself is not freed

### rj_mapfold

* map a function from type rj_mapfold_func over all field-value pairs
  of the given jar
* a pointer to some state is passed to every call

### rj_get, rj_get_next, rj_get_prev

* finds via the given matching criteria the record and returns the value
  belonging to the requested field
* if no record matched the specified default value is returned
* the matching record is memorized

### rj_set, rj_set_next, rj_set_prev

* finds via the given matching criteria the record and replaces the value
  belonging to the requested field
* if no record matched a no-success value is returned
* the matching record is memorized

### rj_add, rj_add_next, rj_add_prev

* finds via the given matching criteria the record and adds the given
  field-value pair to the record
* if no record matched a new record is created with two field-value pairs,
  the key pair and the requested pair
* the matching or created record is memorized

### rj_del_field, rj_del_field_next, rj_del_field_prev

* finds via the given matching criteria the record and removes the field-value
  pair from the record
* if no record matched a no-success value is returned
* if the last field-value pair of the record is removed the record itself is
  removed too
* the matching record is memorized or set to the first of the left records

### rj_del_record, rj_del_record_next, rj_del_record_prev

* finds via the given matching criteria the record and removes the this record
* if no record matched a no-success value is returned
* the memorized mathing record is set to the first of the left records