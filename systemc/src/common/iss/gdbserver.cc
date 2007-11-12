
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include <stdint.h>

#include "common/iss/gdbserver.h"
#include "common/exception.h"

//#define GDBSERVER_DEBUG

namespace soclib { namespace common {

template<typename CpuIss> int GdbServer<CpuIss>::socket_ = -1;
template<typename CpuIss> int GdbServer<CpuIss>::asocket_ = -1;
template<typename CpuIss> typename GdbServer<CpuIss>::State GdbServer<CpuIss>::init_state_ = Running;
template<typename CpuIss> std::vector<GdbServer<CpuIss> *> GdbServer<CpuIss>::list_;
template<typename CpuIss> unsigned int GdbServer<CpuIss>::current_id_ = 0;
template<typename CpuIss> std::map<uint32_t, bool> GdbServer<CpuIss>::break_exec_;
template<typename CpuIss> std::list<GdbWatchPoint> GdbServer<CpuIss>::break_access_;

template<typename CpuIss> uint16_t GdbServer<CpuIss>::port_ = 2346;

template<typename CpuIss>
void GdbServer<CpuIss>::network_init()
{
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socket_ < 0)
        throw soclib::exception::RunTimeError("GdbServer: Unable to create socket");

    {
        int tmp = 1;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
    }

    struct sockaddr_in    addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port_);
    addr.sin_family = AF_INET;

    if (bind(socket_, (struct sockaddr*)&addr, sizeof (struct sockaddr_in)) < 0)
        throw soclib::exception::RunTimeError("GdbServer: Unable to bind()");

    if (listen(socket_, 1) < 0)
        throw soclib::exception::RunTimeError("GdbServer: Unable to listen()");
}

template<typename CpuIss>
GdbServer<CpuIss>::GdbServer(uint32_t ident)
    : CpuIss(ident),
      mem_type_(CpuIss::MEM_NONE),
      mem_count_(0),
      state_(init_state_)
    {
        if (list_.empty())
            network_init();

        id_ = list_.size();
        list_.push_back(this);
    }

template<typename CpuIss>
int GdbServer<CpuIss>::write_packet(char *data_)
{
    unsigned int i, len = strlen(data_);
    char ack, end[4];
    uint8_t chksum = 0;
    char data[len];

    memcpy(data, data_, len + 1);

    // gdb RLE data compression

    unsigned char repeat = 0;
    unsigned int cmplen = len;
    char *cmp = data;
    char *last = 0;

#if 1

    for (i = 0; ; )
        {
            if (i < cmplen && last && (*last == cmp[i]) && (repeat + 29 < 126))
                repeat++;
            else
                {
                    if (repeat > 3)
                        {
                            while (repeat == '#' - 29 ||
                                   repeat == '$' - 29 ||
                                   repeat == '+' - 29 ||
                                   repeat == '-' - 29)
                                {
                                    repeat--;
                                    last++;
                                }
                            last[1] = '*';
                            last[2] = 29 + repeat;
                            memmove(last + 3, cmp + i, cmplen - i + 1);
                            cmp = last + 3;
                            cmplen -= i;
                            i = 0;
                            last = 0;
                            repeat = 0;
                            continue;
                        }
                    else
                        {
                            last = cmp + i;
                            repeat = 0;
                        }

                    if (i == cmplen)
                        break;
                }
            i++;
        }

    cmp[cmplen] = 0;
    len = cmp - data + cmplen;

#endif

    for (i = 0; i < len; i++)
        chksum += data[i];

    sprintf(end, "#%02x", chksum);

    do
        {
#ifdef GDBSERVER_DEBUG
            fprintf(stderr, "[GDB] sending packet data '%s'\n", data);
#endif

            send(asocket_, "$", 1, MSG_DONTWAIT | MSG_NOSIGNAL);
            send(asocket_, data, len, MSG_DONTWAIT | MSG_NOSIGNAL);
            send(asocket_, end, 3, MSG_DONTWAIT | MSG_NOSIGNAL);

            if (read(asocket_, &ack, 1) < 1)
                {
                    close(asocket_);
                    asocket_ = -1;    
                    return -1;
                }
        }
    while (ack != '+');

    return 0;
}

template<typename CpuIss>
char * GdbServer<CpuIss>::read_packet(char *buffer, size_t size)
{
    int res = read(asocket_, buffer, size);

    if (res <= 0)
        {
            close(asocket_);
            asocket_ = -1;    
            return 0;
        }

    uint8_t sum = 0, chksum = 0;
    char *data = 0;
    char *end = 0;
    int i;

    // find data in packet
    for (i = 0; i < res; i++)
        {
            switch (buffer[i])
                {
                case '$':
                    sum = 0;
                    data = buffer + i + 1;
                    break;

                case '#':
                    chksum = sum;
                    end = buffer + i;
                    *end = 0;
                    goto end;

                default:
                    sum += buffer[i];
                }
        }
 end:

    // malformed packets
    if (!data || !end || data >= end)
        return 0;

#ifdef GDBSERVER_DEBUG
    fprintf(stderr, "[GDB] packet with checksum %02x: %s\n", chksum, data);
#endif

    // verify checksum
    end[3] = 0;
    if (chksum != strtoul(end + 1, 0, 16))
        {
            send(asocket_, "-", 1, MSG_DONTWAIT | MSG_NOSIGNAL);
#ifdef GDBSERVER_DEBUG
            fprintf(stderr, "[GDB] bad packet checksum\n");
#endif
            return 0;
        }

    send(asocket_, "+", 1, MSG_DONTWAIT | MSG_NOSIGNAL);

    return data;
}

template<typename CpuIss>
void GdbServer<CpuIss>::process_gdb_packet()
{
    char buffer[1024];

    char *data = read_packet(buffer, 1024);

    if (data)
        {
            switch (data[0])
                {
                case 'k':       // Kill
                    write_packet("OK");
                    cleanup();
                    sc_stop();
                    break;

                case 'D': // Detach
                    write_packet("OK");
                    cleanup();
                    return;

                case 'q': // Query
                    switch (data[1])
                        {
                        case 'C': // get current thread id
                            sprintf(buffer, "QC%x", current_id_ + 1);
                            write_packet(buffer);
                            return;
                        case 'f': // get thread list first
                            if (!strcmp(data + 2, "ThreadInfo"))
                                {
                                    char *b = buffer;
                                    *b++ = 'm';
                                    for (unsigned int i = 0; i < list_.size(); i++)
                                        b += sprintf(b, "%x%s", i + 1, i == list_.size() - 1 ? "" : ",");
                                    write_packet(buffer);
                                    return;
                                }
                            break;

                        case 's': // get thread list next
                            if (!strcmp(data + 2, "ThreadInfo"))
                                {
                                    write_packet("l"); // end of list
                                    return;
                                }
                            break;
                        }
                    break;

                case '?':       // Indicate the reason the target halted
                    write_packet("S05"); // SIGTRAP
                    return;

                case 'p': {       // read single register
                    unsigned int reg = strtoul(data + 1, 0, 16);
                    char fmt[32];

                    sprintf(fmt, "%%0%ux", CpuIss::get_register_size(reg) / 4);
                    sprintf(buffer, fmt, CpuIss::get_register_value(reg));
                    write_packet(buffer);
                    return;
                }

                case 'P': {     // write single register
                    char *end;
                    unsigned int reg = strtoul(data + 1, &end, 16);
                    assert(*end == '=');
                    uint32_t value = strtoul(end + 1, 0, 16);

                    CpuIss::set_register_value(reg, value);
                    write_packet("OK");
                    return;
                }

                case 'g': {      // read all registers
                    char *b = buffer;
                    for (unsigned int i = 0; i < CpuIss::get_register_count(); i++)
                        {
                            char fmt[32];
                            sprintf(fmt, "%%0%ux", CpuIss::get_register_size(i) / 4);
                            b += sprintf(b, fmt, CpuIss::get_register_value(i));
                        }
                    write_packet(buffer);
                    return;
                }

                case 'G': {       // write all registers

                    data++;

                    for (unsigned int i = 0; i < CpuIss::get_register_count(); i++)
                        {
                            size_t s = CpuIss::get_register_size(i) / 4;
                            char word[s + 1];
                            word[s] = 0;
                            memcpy(word, data, s);
                            if (strlen(word) != s)
                                break;
                            CpuIss::set_register_value(i, strtoul(word, 0, 16));
                            data += s;
                        }

                    write_packet("OK");

                    return;
                }

                case 'm': {     // read memory
                    char *end;
                    uint32_t addr = strtoul(data + 1, &end, 16);
                    assert(*end == ',');
                    size_t len = strtoul(end + 1, 0, 16);

                    mem_error_ = 0;
                    mem_type_ = CpuIss::MEM_LB;
                    mem_addr_ = addr;
                    mem_len_ = mem_count_ = len;
                    mem_buff_ = mem_ptr_ = (uint8_t*)malloc(len);

                    return;
                }

                case 'M': {      // write memory
                    char *end;
                    uint32_t addr = strtoul(data + 1, &end, 16);
                    assert(*end == ',');
                    size_t len = strtoul(end + 1, &end, 16);
                    assert(*end == ':');

                    mem_error_ = 0;
                    mem_type_ = CpuIss::MEM_SB;
                    mem_addr_ = addr;
                    mem_len_ = mem_count_ = len;
                    mem_buff_ = mem_ptr_ = (uint8_t*)malloc(len);

                    char byte[3] = { 0 };

                    for (unsigned int i = 0; i < len; i++)
                        {
                            memcpy(byte, end + 1 + i * 2, 2);
                            mem_buff_[i] = strtoul(byte, 0, 16);
                        }

                    mem_data_ = *mem_ptr_++;

                    return;
                }

                case 'c': {      // continue [optional resume addr in hex]
                    if (data[1])
                        CpuIss::r_pc = strtoul(data + 1, 0, 16);

                    change_all_states(Running);
                    return;
                }

                case 's': {      // continue single step [optional resume addr in hex]
                    if (data[1])
                        CpuIss::r_pc = strtoul(data + 1, 0, 16);

                    state_ = Step;
                    return;
                }

                case 'H': {     // set current thread
                    int id = strtol(data + 2, 0, 16);

                    switch (id)
                        {
                        case -1: // All threads
                        case 0: // pick any
                            break;
                        default:
                            if ((unsigned)id <= list_.size())
                                current_id_ = id - 1;
                            break;
                        }

#ifdef GDBSERVER_DEBUG
                    fprintf(stderr, "[GDB] thread %i selected\n", current_id_);
#endif
                    write_packet("OK");

                    return;
                }

                case 'T': {     // check if thread is alive
                    int id = strtol(data + 1, 0, 16);
                    if (id > 0 && (unsigned)id <= list_.size())
                        write_packet("OK");
                    else
                        write_packet("E2");
                    return;
                }

                case 'z':       // set and clean break points
                case 'Z': {
                    char *end;
                    uint32_t addr = strtoul(data + 3, &end, 16);
                    assert(*end == ',');
                    size_t len = strtoul(end + 1, 0, 16);

                    switch (data[1])
                        {
                        case '0':
                        case '1': // execution break point
                            if (data[0] == 'Z')
                                break_exec_[addr] = true;
                            else
                                break_exec_.erase(addr);
                            break;

                        case '2': // write watch point
                            if (data[0] == 'Z')
                                access_break_add(GdbWatchPoint(addr, len, GdbWatchPoint::a_write));
                            else
                                access_break_remove(GdbWatchPoint(addr, len, GdbWatchPoint::a_write));
                            break;

                        case '3': // read watch point
                            if (data[0] == 'Z')
                                access_break_add(GdbWatchPoint(addr, len, GdbWatchPoint::a_read));
                            else
                                access_break_remove(GdbWatchPoint(addr, len, GdbWatchPoint::a_read));
                            break;

                        case '4': // access watch point
                            if (data[0] == 'Z')
                                access_break_add(GdbWatchPoint(addr, len, GdbWatchPoint::a_rw));
                            else
                                access_break_remove(GdbWatchPoint(addr, len, GdbWatchPoint::a_rw));
                            break;

                        default:
                            write_packet("");
                            return;
                        }

                    write_packet("OK");
                    return;
                }

                }

            // empty reply if not supported
            write_packet("");
        }
}

template<typename CpuIss>
void GdbServer<CpuIss>::try_accept()
{
    struct pollfd pf;

    pf.fd = socket_;
    pf.events = POLLIN | POLLPRI;

    if (poll(&pf, 1, 0) > 0)
        {
            struct sockaddr_in addr;
            socklen_t addr_size = sizeof(addr);

            asocket_ = accept(socket_, (struct sockaddr*)&addr, &addr_size);

            if (asocket_ >= 0)
                {
                    // freeze all processors on new connections
                    change_all_states(MemWait);
                }
        }
}

template<typename CpuIss>
bool GdbServer<CpuIss>::process_mem_access()
{
    if (mem_error_)
        {
            write_packet("E0d");

            free(mem_buff_);
            mem_type_ = CpuIss::MEM_NONE;
            mem_count_ = 0;
            return false;
        }

    switch (mem_type_)
        {
        case CpuIss::MEM_LB: {
            do
                {
                    *mem_ptr_++ = mem_data_ >> (8 * (mem_addr_ & 3));
                    mem_addr_++;
                    mem_count_--;
                }
            while (mem_count_ && (mem_addr_ & 3));

            if (mem_count_)
                return true;

            char packet[mem_len_ * 2 + 1];
            char *b = packet;

            for (unsigned int i = 0; i < mem_len_ ; i++)
                b += sprintf(b, "%02x", mem_buff_[i]);

            write_packet(packet);

            free(mem_buff_);
            mem_type_ = CpuIss::MEM_NONE;

            break;
        }

        case CpuIss::MEM_SB: {
            mem_addr_++;
            mem_count_--;

            if (mem_count_)
                {
                    mem_data_ = *mem_ptr_++;
                    return true;
                }

            write_packet("OK");
            free(mem_buff_);
            mem_type_ = CpuIss::MEM_NONE;

            break;
        }

        default:
            assert(0);
        }

    return false;
}

template<typename CpuIss>
void GdbServer<CpuIss>::watch_mem_access()
{
    if (!break_access_.empty())
        {
            Iss::DataAccessType type;
            uint32_t address, data;

            uint8_t mask;
            CpuIss::getDataRequest(type, address, data);

            switch(type)
                {
                default:
                    mask = 0;
                    break;

                case Iss::MEM_SB:
                case Iss::MEM_SH:
                case Iss::MEM_SW:
                    mask = GdbWatchPoint::a_write;
                    break;

                case Iss::MEM_LB:
                case Iss::MEM_LBU:
                case Iss::MEM_LH:
                case Iss::MEM_LHBR:
                case Iss::MEM_LHU:
                case Iss::MEM_LWBR:
                case Iss::MEM_LW:
                    mask = GdbWatchPoint::a_read;
                    break;
                }

            if (mask && (mask = access_break_check(address, mask)))
                {
                    change_all_states(MemWait); // all processors will end their memory access
                    state_ = Frozen; // except the current processor
                    current_id_ = id_;
                    const char *reason;

                    switch (mask)
                        {
                        case (GdbWatchPoint::a_write):
                            reason = "watch";
                            break;

                        case (GdbWatchPoint::a_read):
                            reason = "rwatch";
                            break;

                        default:
                            reason = "awatch";
                            break;
                        }

                    char buffer[32];
                    sprintf(buffer, "T05thread:%x;%s:%x;", id_ + 1, reason, address);
                    write_packet(buffer);
                }
        }
}

template<typename CpuIss>
bool GdbServer<CpuIss>::check_break_points()
{
    if (break_exec_.find(CpuIss::r_pc) != break_exec_.end())
        {
            change_all_states(MemWait);
            current_id_ = id_;

            char buffer[32];
            sprintf(buffer, "T05thread:%x;", id_ + 1);
            write_packet(buffer);

            return true;
        }

    return false;
}

template<typename CpuIss>
void GdbServer<CpuIss>::cleanup()
{
    for (unsigned int i = 0; i < list_.size(); i++)
        {
            GdbServer & gs = *list_[i];

            gs.mem_type_ = CpuIss::MEM_NONE;
            if (gs.mem_count_)
                free(gs.mem_buff_);

            gs.break_exec_.clear();
            gs.break_access_.clear();
            gs.state_ = Running;
        }

    close(asocket_);
    asocket_ = -1;
}

template<typename CpuIss>
void GdbServer<CpuIss>::step()
{
    // check for incoming connection
    if (id_ == 0)
        {
            static unsigned int counter = 0;

            if (asocket_ < 0 && !(counter++ % 128))
                try_accept();
        }

    if (state_ == MemWait)
        state_ = Frozen;      // no more memory access pending when step() is called

    if (state_ == Frozen)
        {
            if (id_ == current_id_)
                {
                    // process memory access
                    if (mem_count_ > 0 && process_mem_access())
                        return;

                    // process incoming packets
                    if (asocket_ >= 0)
                        {
                            struct pollfd pf;

                            pf.fd = asocket_;
                            pf.events = POLLIN | POLLPRI;

                            switch (poll(&pf, 1, 0))
                                {
                                case 0:         // nothing happened
                                    break;
                                case 1:         // need to read data
                                    process_gdb_packet();
                                    break;
                                default:
                                    cleanup();
                                    break;
                                }
                        }
                }
        }
    else
        {
            // check execution break point
            if (check_break_points())
                return;

            CpuIss::step();

            // check memory access break point
            watch_mem_access();

            if (state_ == Step) // single step execution ends here
                {
                    char buffer[32];
                    sprintf(buffer, "T05thread:%x;", id_ + 1);
                    write_packet(buffer);
                    state_ = MemWait;
                }
        }
}

}}


// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4
