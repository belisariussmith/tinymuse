# TinyMUSE Server Architecture

## Technical Implementation Details

This document describes the technical implementation of the TinyMUSE server, including low-level details about networking, database storage, memory management, and system integration.

## Server Process Model

### Single-Process Architecture
TinyMUSE runs as a single, multi-threaded process that handles all client connections and game logic:

```
┌─────────────────────────────────────────────────────────────┐
│                    TinyMUSE Server Process                   │
├─────────────────────────────────────────────────────────────┤
│  Main Loop (game.c)                                         │
│  ├─ Accept new connections                                   │
│  ├─ Read input from all clients                             │
│  ├─ Process commands                                         │
│  ├─ Send output to clients                                   │
│  └─ Periodic housekeeping                                    │
├─────────────────────────────────────────────────────────────┤
│  Database Layer (db.c)                                      │
│  ├─ Object storage and retrieval                            │
│  ├─ Attribute management                                     │
│  ├─ Database integrity checks                               │
│  └─ Periodic database saves                                  │
├─────────────────────────────────────────────────────────────┤
│  Network Layer (io.bsd.c)                                   │
│  ├─ Socket management                                        │
│  ├─ Connection handling                                      │
│  ├─ Input/output buffering                                   │
│  └─ Protocol handling                                        │
└─────────────────────────────────────────────────────────────┘
```

### Main Server Loop
The server operates on a select()-based event loop:

1. **Connection Management**: Accept new connections on the listening socket
2. **Input Processing**: Read available data from all client sockets
3. **Command Execution**: Process queued commands from clients
4. **Output Transmission**: Send responses to clients
5. **Periodic Tasks**: Database saves, cleanup, statistics updates

## Network Architecture

### Socket Management
- **Technology**: BSD Sockets with select() for I/O multiplexing
- **Protocol**: TCP/IP with text-based command protocol
- **Port**: Default port 4201 (configurable)
- **Connections**: Supports multiple simultaneous client connections

### Connection Lifecycle
```
Client Connect → Authentication → Session Active → Command Processing → Disconnect
      ↓              ↓                ↓                    ↓               ↓
   Socket Accept   Login Check    Add to Player List   Command Loop    Cleanup
```

### Protocol Details
- **Format**: Line-based text protocol
- **Encoding**: ASCII/UTF-8 text
- **Commands**: Single-line commands with arguments
- **Responses**: Multi-line text responses
- **Special Characters**: Various tokens for commands (@, ", :, etc.)

## Database Architecture

### File-Based Storage
The database is stored in a human-readable text format:

```
Database File Structure:
├─ Header (version, metadata)
├─ Object Records
│  ├─ Object #0 (Room)
│  ├─ Object #1 (Thing)
│  ├─ Object #2 (Exit)
│  └─ Object #3 (Player)
└─ Footer (end markers)
```

### Object Storage Format
Each object is stored with the following structure:
```
&<dbref>
<name>
<location>
<zone>
<contents>
<exits>
<link>
<next>
<owner>
[<powers>]  // Only for players
<flags>
<createtime>
<modtime>
<attributes>
<parents>
<children>
<attribute_definitions>
\
```

### Database Operations
- **Loading**: Entire database loaded into memory at startup
- **Saving**: Periodic full database dumps to disk
- **Integrity**: Database consistency checks on startup
- **Backup**: Automatic backup creation during saves
- **Migration**: Version-based database format upgrades

## Memory Management

### Object Storage
- **Primary Storage**: Array of object structures in memory
- **Free List**: Linked list of deleted/recycled objects
- **Memory Pool**: Custom memory allocation for strings and attributes
- **Garbage Collection**: Manual cleanup of unused objects

### Buffer Management
- **Input Buffers**: Per-connection input buffers
- **Output Buffers**: Per-connection output queues
- **String Buffers**: Temporary string manipulation buffers
- **Command Buffers**: Command parsing and execution buffers

### Memory Allocation Strategy
```c
// Custom memory allocator (nalloc.c)
typedef struct {
    void *memory_pool;
    size_t pool_size;
    size_t used;
    struct free_block *free_list;
} memory_manager;
```

## Command Processing Architecture

### Command Pipeline
1. **Input Reception**: Raw text received from client
2. **Tokenization**: Command split into tokens
3. **Command Recognition**: Command lookup in command table
4. **Permission Check**: Verify user has required permissions
5. **Argument Parsing**: Parse command arguments
6. **Execution**: Call appropriate command handler
7. **Response Generation**: Generate response messages
8. **Output Queuing**: Queue responses for transmission

### Command Handler Structure
```c
typedef struct {
    char *name;           // Command name
    void (*handler)();    // Function pointer to handler
    int min_args;         // Minimum arguments required
    int permissions;      // Required permissions
    char *help_text;      // Help documentation
} command_table_entry;
```

## User Authentication and Security

### Authentication Flow
1. **Connection**: Client connects to server
2. **Welcome**: Server sends welcome message
3. **Login Prompt**: Server requests username/password
4. **Credential Check**: Server verifies credentials
5. **Session Creation**: Server creates user session
6. **Permission Setup**: Server loads user permissions

### Permission System
- **Classes**: Hierarchical user classes (Guest → Visitor → Citizen → Builder → Admin)
- **Powers**: Granular permissions for specific actions
- **Flags**: Object-level access controls
- **Locks**: Attribute-based access restrictions

### Password Security
- **Encryption**: Passwords stored using crypt() function
- **Validation**: Secure password comparison
- **Aging**: No automatic password expiration
- **Strength**: No enforced password complexity requirements

## Inter-Process Communication

### Signal Handling
- **SIGTERM**: Graceful shutdown
- **SIGUSR1**: Database dump
- **SIGUSR2**: Statistics report
- **SIGCHLD**: Child process cleanup

### File-Based Communication
- **Lock Files**: Prevent multiple server instances
- **Log Files**: Debug and error logging
- **Configuration**: Compile-time configuration headers

## Performance Characteristics

### Scalability Limits
- **Max Connections**: Limited by file descriptor limits
- **Memory Usage**: Grows with database size
- **CPU Usage**: Single-threaded command processing
- **Disk I/O**: Periodic database saves can cause latency spikes

### Optimization Strategies
- **Caching**: Object and attribute caching
- **Lazy Loading**: Attributes loaded on demand
- **Buffer Management**: Efficient string handling
- **Connection Pooling**: Reuse of connection structures

## System Integration

### File System Layout
```
tinymuse/
├─ bin/
│  └─ tinymuse              // Server executable
├─ run/
│  ├─ db/
│  │  └─ mdb                // Database file
│  ├─ logs/                 // Log files
│  ├─ msgs/                 // Message files
│  │  ├─ welcome.txt
│  │  ├─ connect.txt
│  │  └─ helptext
│  └─ lockouts/             // Access control
└─ src/                     // Source code
```

### Configuration Management
- **Compile-Time**: Configuration through header files
- **Runtime**: Limited runtime configuration options
- **Defaults**: Sensible defaults for most installations

### Logging and Monitoring
- **Log Files**: Separate logs for different subsystems
- **Debug Output**: Compile-time debug flags
- **Statistics**: Runtime performance statistics
- **Error Reporting**: Comprehensive error logging

## Deployment Architecture

### Installation Requirements
- **OS**: Unix/Linux system
- **Compiler**: GCC with C99 support
- **Libraries**: Standard C library, BSD sockets, crypt
- **Permissions**: Write access to database and log directories

### Startup Process
1. **Initialization**: Load configuration and initialize subsystems
2. **Database Load**: Load complete database into memory
3. **Network Setup**: Open listening socket
4. **Signal Handlers**: Install signal handlers
5. **Main Loop**: Enter main event loop

### Shutdown Process
1. **Signal Reception**: Receive shutdown signal
2. **Connection Cleanup**: Close all client connections
3. **Database Save**: Save current state to disk
4. **Resource Cleanup**: Free allocated memory
5. **Process Exit**: Clean process termination

## Security Considerations

### Network Security
- **No Encryption**: Plain text protocol (security by obscurity)
- **Access Control**: IP-based connection filtering
- **Rate Limiting**: Basic flood protection
- **Input Validation**: Limited input sanitization

### Database Security
- **File Permissions**: Relies on file system permissions
- **Backup Security**: No encryption of database backups
- **Access Control**: Application-level permission system
- **Audit Trail**: Basic logging of administrative actions

## Legacy Considerations

### Historical Context
- **Era**: Developed in 1990s when security was less of a concern
- **Network**: Designed for trusted network environments
- **Scale**: Optimized for small to medium communities
- **Hardware**: Designed for limited memory and CPU resources

### Modernization Requirements
- **Encryption**: Add TLS/SSL support
- **Authentication**: Implement modern authentication methods
- **Scalability**: Multi-process or multi-threaded architecture
- **Security**: Comprehensive input validation and sanitization
- **Protocol**: RESTful API for modern web clients

This server architecture provides a solid foundation for a text-based multi-user environment while reflecting the design constraints and priorities of its era.