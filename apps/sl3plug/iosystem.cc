/***************************************************************************
 * Copyright (C) GFZ Potsdam                                               *
 * All rights reserved.                                                    *
 *                                                                         *
 * GNU Affero General Public License Usage                                 *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 ***************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <algorithm>
#include <new>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "fsdh.h"
#include "libmseed.h"

#include "iosystem.h"
#include "confbase.h"
#include "cppstreams.h"
#include "utils.h"
#include "descriptor.h"
#include "buffer.h"
#include "diag.h"

namespace IOSystem_private {

using namespace std;
using namespace CPPStreams;
using namespace CfgParser;
using namespace Utilities;

Fdset fds;

//*****************************************************************************
// Sequence
//*****************************************************************************

class Sequence
  {
  friend ostream &operator<<(ostream &s, const Sequence &x);
  private:
    int value;

  public:
    enum { size = 6, mask = 0xffffff, uninitialized = -1 };
    
    Sequence(): value(uninitialized) {}
    
    Sequence(int value_init): value(value_init)
      {
        internal_check(value == uninitialized || !(value & ~mask));
      }

    void increment()
      {
        internal_check(value != uninitialized);
        value = (value + 1) & mask;
      }

    Sequence operator+(int n) const
      {
        internal_check(value != uninitialized && !(n & ~mask));
        return ((value + n) & mask);
      }

    Sequence operator-(int n) const
      {
        internal_check(value != uninitialized && !(n & ~mask));
        return ((value - n) & mask);
      }

    operator int() const
      {
        return value;
      }
  };

ostream &operator<<(ostream &s, const Sequence &x)
  {
    ios_base::fmtflags flags_saved = s.flags(ios_base::hex | ios_base::uppercase);
    char filler_saved = s.fill('0');
  
    internal_check(x != Sequence::uninitialized);
    s << setw(x.size) << x.value;
    s.flags(flags_saved);
    s.fill(filler_saved);
    return s;
  }

// ios_base::hex doesn't seem to work for input streams in libstdc++ 2.90.7
#ifdef USE_HEX_ISTREAM
istream &operator>>(istream &s, Sequence &x)
  {
    int testx;
    ios_base::fmtflags flags_saved = s.flags(ios_base::hex);
  
    if(s >> testx)
      {
        if(testx & ~Sequence::mask) s.clear(ios_base::badbit);
        else x = testx;
      }

    s.flags(flags_saved);
    return s;
  }

#else
istream &operator>>(istream &s, Sequence &x)
  {
    int testx;
    char *tail;
    string buf;

    if(s >> setw(Sequence::size) >> buf)
      {
        testx = strtoul(buf.c_str(), &tail, 16);
        if(*tail || (testx & ~Sequence::mask)) s.clear(ios_base::badbit);
        else x = testx;
      }

    return s;
  }
#endif

//*****************************************************************************
// BufferImpl
//*****************************************************************************

class BufferImpl: public Buffer
  {
  friend class BufferStoreImpl;
  private:
    BufferImpl *prevptr;
    BufferImpl *nextptr;
    void *dataptr;
    Sequence seq;

    ~BufferImpl()
      {
        free(dataptr);
      }
    
  public:
    BufferImpl(int size): Buffer(size), prevptr(NULL), nextptr(NULL), dataptr(NULL)
      {
        if((dataptr = malloc(size)) == NULL) throw bad_alloc();
      }

    BufferImpl *next() const
      {
        return nextptr;
      }

    void *data() const
      {
        return dataptr;
      }

    Sequence sequence() const
      {
        return seq;
      }
  };

//*****************************************************************************
// BufferStoreImpl
//*****************************************************************************

class BufferStoreImplPartner
  {
  public:
    virtual void new_buffer(BufferImpl *buf) =0;
    virtual void delete_oldest_buffer(BufferImpl *buf) =0;
    virtual ~BufferStoreImplPartner() {}
  };
  
class BufferStoreImpl: public BufferStore
  {
  private:
    BufferStoreImplPartner &partner;
    BufferImpl *buf_head, *buf_free, *buf_first, *buf_queue;
    Sequence seq;
    int bufsize;
    int nbufs;

    void next_buffer();
    
    void insert_buffer_after(BufferImpl *a, BufferImpl *b)
      {
        a->prevptr = b;

        if(b != NULL)
          {
            a->nextptr = b->nextptr;
            b->nextptr = a;
          }
        else
          {
            a->nextptr = NULL;
          }

        if(a->nextptr) a->nextptr->prevptr = a;
      }

    void insert_buffer_before(BufferImpl *a, BufferImpl *b)
      {
        a->nextptr = b;

        if(b != NULL)
          {
            a->prevptr = b->prevptr;
            b->prevptr = a;
          }
        else
          {
            a->prevptr = NULL;
          }

        if(a->prevptr) a->prevptr->nextptr = a;
      }

    void remove_buffer(BufferImpl *a)
      {
        if(a->prevptr) a->prevptr->nextptr = a->nextptr;
        if(a->nextptr) a->nextptr->prevptr = a->prevptr;
      }

  public:
    BufferStoreImpl(BufferStoreImplPartner &partner_init, int bufsize_init,
      int nbufs_init);
    ~BufferStoreImpl();
    Buffer *get_buffer();
    void queue_buffer(Buffer *buf1);
    void load_buffers(int fd);
    void create_blank_buffers(int n);
    void enlarge(int newsize);
    
    BufferImpl *first() const
      {
        return buf_first;
      }

    int size() const
      {
        return nbufs;
      }

    void init_seq(Sequence seq_init)
      {
        seq = seq_init;
      }
    
    Sequence end_seq() const
      {
        return seq;
      }
  };

BufferStoreImpl::BufferStoreImpl(BufferStoreImplPartner &partner_init, int bufsize_init,
  int nbufs_init): partner(partner_init), buf_first(NULL), seq(0), bufsize(bufsize_init),
  nbufs(nbufs_init)
  {
    internal_check(nbufs >= 2);
    
    buf_head = new BufferImpl(bufsize);
    buf_head->prevptr = buf_head->nextptr = NULL;
    buf_queue = new BufferImpl(bufsize);
    insert_buffer_after(buf_queue, buf_head);
    buf_free = buf_head;

    for(int i = 2; i < nbufs; ++i)
      {
        BufferImpl* buf = new BufferImpl(bufsize);
        insert_buffer_after(buf, buf_free);
      }
  }
        
BufferStoreImpl::~BufferStoreImpl()
  {
    while(buf_head)
      {
        BufferImpl* buf = buf_head;
        buf_head = buf->nextptr;
        delete buf;
      }
  }

Buffer *BufferStoreImpl::get_buffer(void)
  {
    internal_check(buf_free != NULL);
    
    BufferImpl* buf = buf_free;
    buf_free = buf_free->nextptr;
    if(buf_first == buf) buf_first = buf->nextptr;
    if(buf->seq != Sequence::uninitialized) partner.delete_oldest_buffer(buf);
    buf->seq = Sequence::uninitialized;
    return(buf);
  }

void BufferStoreImpl::queue_buffer(Buffer *buf1)
  {
    BufferImpl *buf = dynamic_cast<BufferImpl *>(buf1);
    internal_check(buf != NULL);

    buf->seq = seq;
    seq.increment();
    if(buf_head == buf) buf_head = buf->nextptr;
    remove_buffer(buf);
    insert_buffer_after(buf, buf_queue);
    buf_queue = buf;
    if(buf_first == NULL) buf_first = buf;
    partner.new_buffer(buf);
  }

void BufferStoreImpl::next_buffer()
  {
    internal_check(buf_free != NULL);
    
    BufferImpl* buf = buf_free;
    buf_free = buf_free->nextptr;
    buf->seq = seq;
    seq.increment();
    if(buf_head == buf) buf_head = buf->nextptr;
    remove_buffer(buf);
    insert_buffer_after(buf, buf_queue);
    buf_queue = buf;
    if(buf_first == NULL) buf_first = buf;
  }

void BufferStoreImpl::load_buffers(int fd)
  {
    internal_check(buf_free != NULL);

    int r;
    while((r = read(fd, buf_free->dataptr, buf_free->size)) == buf_free->size)
        next_buffer();

    internal_check(r == 0);
  }

void BufferStoreImpl::create_blank_buffers(int n)
  {
    internal_check(buf_free != NULL);

    for(int i = 0; i < n; ++i)
      {
        BufferImpl* buf = buf_free;
        memset(buf->dataptr, 0, buf->size);
        next_buffer();
        partner.new_buffer(buf);
      }
  }

void BufferStoreImpl::enlarge(int newsize)
  {
    internal_check(buf_free != NULL);

    while(nbufs < newsize)
      {
        BufferImpl* buf = new BufferImpl(bufsize);
        insert_buffer_after(buf, buf_free);
        ++nbufs;
      }
  }

//*****************************************************************************
// StationIO
//*****************************************************************************

class StationIO;

class StationIOPartner
  {
  public:
    virtual void notify(StationIO *station) =0;
  };

class StationIO: private BufferStoreImplPartner
  {
  private:
    StationIOPartner &partner;
    rc_ptr<BufferStoreImpl> bufs;
    BufferImpl *buf;

    // BufferStoreImpl callbacks
    void new_buffer(BufferImpl *b);
    void delete_oldest_buffer(BufferImpl *b);

    int send_packet(int fd, const void *data, uint32_t len, char format, char subformat);

  public:
    const string station_key;

    StationIO(StationIOPartner &partner_init,
      const string &station_key_init, int nbufs);

    bool flush(int fd);

    rc_ptr<BufferStore> get_bufs()
      {
        return bufs;
      }
  };

// Callback from BufferStoreImpl (through BufferStoreImplPartner) to signal
// that a new buffer (eg., Mini-SEED record) has arrived.

void StationIO::new_buffer(BufferImpl *b)
  {
    if(buf == NULL)
        buf = b;

    partner.notify(this);
  }

// Callback from BufferStoreImpl (through BufferStoreImplPartner) to signal
// that the oldest record in BufferStore ("memory buffer") will be deleted.

void StationIO::delete_oldest_buffer(BufferImpl *b)
  {
    if(buf == b)
        buf = buf->next();
  }

int StationIO::send_packet(int fd, const void *data, uint32_t len, char format, char subformat)
  {
    // TODO: byteorder
    int64_t minus1 = -1;
    string packet = "SE" + string(&format, 1) + string(&subformat, 1) +
      string((char*)&len, 4) + string((char*)&minus1, 8) + string("", 1) +
      string((const char*)data, len);

    return send(fd, packet.data(), packet.size(), 0);
  }

StationIO::StationIO(StationIOPartner &partner_init,
  const string &station_key_init, int nbufs):
  partner(partner_init), buf(NULL), station_key(station_key_init)
  {
    bufs = new BufferStoreImpl(*this, (1 << MSEED_RECLEN), nbufs);
  }

bool StationIO::flush(int fd)
  {
    MS3Record *msr = NULL;

    while(buf != NULL)
      {
        int err;
        if((err = msr3_parse((const char *)buf->data(), buf->size, &msr, MSF_UNPACKDATA, 0)) != MS_NOERROR)
          {
            logs(LOG_ERR) << station_key << ": " << ms_errorstr(err) << endl;
            buf = buf->next();
            if (msr) msr3_free(&msr);
            continue;
          }

        char subformat;

        if(msr->formatversion == 2)
          {
            subformat = 'D';

            if (msr->samplecnt == 0)
              {
                if(mseh_exists(msr, "/FDSN/Event/Detection"))
                  {
                    subformat = 'E';
                  }
                else if(mseh_exists(msr, "/FDSN/Calibration/Sequence"))
                  {
                    subformat = 'C';
                  }
                else if(mseh_exists(msr, "/FDSN/Time/Exception"))
                  {
                    subformat = 'T';
                  }
                else
                  {
                    subformat = 'O';  // opaque
                  }
              }
            else if(msr->samprate == 0.0 && msr->sampletype == 't')
              {
                subformat = 'L';  // log
              }
          }
        else if( msr->formatversion == 3)
          {
            subformat = 'D';
          }
        else
          {
            logs(LOG_ERR) << station_key << ": unsupported MiniSEED version: " <<
              msr->formatversion << endl;

            buf = buf->next();
            msr3_free(&msr);
            continue;
          }

        if(send_packet(fd, buf->data(), buf->size, '2', subformat) < 0)
          {
            logs(LOG_ERR) << strerror(errno) << endl;
            msr3_free(&msr);
            return false;
          }

        if(subformat != 'D')
          {
            buf = buf->next();
            msr3_free(&msr);
            continue;
          }

        msr->formatversion = 3;
        msr->reclen = -1;

        struct Handlerdata
          {
            int fd;
            int err;
            StationIO *obj;
          } h;

        h.fd = fd;
        h.err = 0;
        h.obj = this;

        msr3_pack(msr, [](char *record, int reclen, void *handlerdata)
          {
            Handlerdata* h = reinterpret_cast<Handlerdata *>(handlerdata);
            h->err = h->obj->send_packet(h->fd, record, reclen, '3', 'D');
          }, &h, NULL, MSF_FLUSHDATA, 0);

        if(h.err < 0)
          {
            logs(LOG_ERR) << strerror(errno) << endl;
            msr3_free(&msr);
            return false;
          }

        buf = buf->next();
        msr3_free(&msr);
      }

    return true;
  }

//*****************************************************************************
// ConnectionManagerImpl
//*****************************************************************************

class ConnectionManagerImpl: public ConnectionManager,
  private StationIOPartner
  {
  private:
    map<StationDescriptor, rc_ptr<StationIO> > stations;
    set<StationIO *> pending;
    int fd;

    // StationIO callback
    void notify(StationIO *station);

    bool open_feed(int port);

  public:
    ConnectionManagerImpl();
      
    rc_ptr<BufferStore> register_station(const string &station_key,
      const string &station_name, const string &network_id, int nbufs);

    void start(int port);
    void save_state();
    void restore_state();
  };

// Callback from StationIO (through StationIOPartner) to signal
// that new data is available.

void ConnectionManagerImpl::notify(StationIO *station)
  {
    pending.insert(station);
  }

bool ConnectionManagerImpl::open_feed(int port)
  {
    sockaddr_in saddr;

    internal_check(fd = socket(AF_INET, SOCK_STREAM, 0));
    internal_check(fcntl(fd, F_SETFD, FD_CLOEXEC) >= 0);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;

    if(connect(fd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0)
      {
        logs(LOG_WARNING) << "error connecting 127.0.0.1:" << port << ": " 
          << strerror(errno) << endl;

        close(fd);
        fd = -1;
        return false;
      }

    if(send(fd, "FEED\r\n", 6, 0) < 0)
      {
        logs(LOG_WARNING) << "error sending FEED to 127.0.0.1:" << port << ": "
          << strerror(errno) << endl;

        close(fd);
        fd = -1;
        return false;
      }

    size_t len;
    char msg[10];

    if((len = recv(fd, msg, sizeof(msg), 0)) < 0)
      {
        logs(LOG_WARNING) << "error receiving response from 127.0.0.1:" << port << ": "
          << strerror(errno) << endl;

        close(fd);
        fd = -1;
        return false;
      }

    if(string(msg, 0, len) != "OK\r\n")
      {
        logs(LOG_WARNING) << "FEED not accepted by 127.0.0.1:" << port << ": "
          << string(msg, 0, len) << endl;

        close(fd);
        fd = -1;
        return false;
      }

    logs(LOG_NOTICE) << "feed to 127.0.0.1:" << port << " opened successfully" << endl;
    return true;
  }
    
ConnectionManagerImpl::ConnectionManagerImpl(): fd(-1)
  {
    handler = new ConnectionHandler;
  }
      
rc_ptr<BufferStore> ConnectionManagerImpl::register_station(const string &station_key,
  const string &station_name, const string &network_id, int nbufs)
  {
    StationDescriptor sd(network_id, station_name);
    rc_ptr<StationIO> station = new StationIO(*this, station_key, nbufs);

    if(!stations.insert(make_pair(sd, station)).second)
        return NULL;
        
    return station->get_bufs();
  }

void ConnectionManagerImpl::start(int port)
  {
    while((*handler)(fds))
      {
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        fds.select(&tv);

        if(fds.status() == 0 || (fds.status() < 0 && errno == EINTR)) continue;

        if(fds.status() < 0) throw LibraryError("select error");

        if(fd == -1 && !open_feed(port))
            continue;

        while(!pending.empty())
          {
            StationIO* station = *pending.begin();

            if(!station->flush(fd))
              {
                close(fd);
                fd = -1;
                break;
              }

            pending.erase(station);
          }
      }

    logs(LOG_INFO) << "exit" << endl;

    if(fd != -1)
        close(fd);
  }

void ConnectionManagerImpl::save_state()
  {
  }

void ConnectionManagerImpl::restore_state()
  {
  }

void log_warn(const char *message)
  {
    logs(LOG_WARNING) << message << endl;
  }

//*****************************************************************************
// Entry Point
//*****************************************************************************

rc_ptr<ConnectionManager> make_conn_manager()
  {
    ms_loginit(log_warn, NULL, log_warn, NULL);
    return new ConnectionManagerImpl();
  }

} // namespace IOSystem_private

