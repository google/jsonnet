/*
Copyright 2015 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef STATIC_ERROR_H
#define STATIC_ERROR_H

struct Location {
    unsigned long line;
    unsigned long column;
    Location(void)
      : line(0), column(0)
    { }
    Location(unsigned long line_number, unsigned long column)
      : line(line_number), column(column)
    { }
    bool isSet(void) const
    {
        return line != 0;
    }
};

static inline std::ostream &operator<<(std::ostream &o, const Location &loc)
{
    o << loc.line << ":" << loc.column;
    return o;
}

struct LocationRange {
    std::string file;
    Location begin, end;
    LocationRange(void)
    { }
    /** This is useful for special locations, e.g. manifestation entry point. */
    LocationRange(const std::string &msg)
      : file(msg)
    { }
    LocationRange(const std::string &file, const Location &begin, const Location &end)
      : file(file), begin(begin), end(end)
    { }
    bool isSet(void) const
    {
        return begin.isSet();
    }
};

static inline std::ostream &operator<<(std::ostream &o, const LocationRange &loc)
{
    if (loc.file.length() > 0)
        o << loc.file;
    if (loc.isSet()) {
        if (loc.file.length() > 0)
            o << ":";
        if (loc.begin.line == loc.end.line) {
            if (loc.begin.column == loc.end.column) {
                o << loc.begin;
            } else {
                o << loc.begin << "-" << loc.end.column;
            }
        } else {
            o << "(" << loc.begin << ")-(" << loc.end << ")";
        }
    }
    return o;
}

struct StaticError {
    LocationRange location;
    std::string msg;
    StaticError(const std::string &msg)
      : msg(msg)
    {
    }
    StaticError(const std::string &filename, const Location &location, const std::string &msg)
      : location(filename, location, location), msg(msg)
    {
    }
    StaticError(const LocationRange &location, const std::string &msg)
      : location(location), msg(msg)
    {
    }
};

static inline std::ostream &operator<<(std::ostream &o, const StaticError &err)
{
    if (err.location.isSet()) {
        o << err.location << ":";
    }
    o << " " << err.msg;
    return o;
}

#endif
