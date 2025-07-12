# TinyMUSE Software Architecture

## Overview

TinyMUSE is a Multi-User Simulated Environment (MUSE) server originally developed in the 1990s. It provides a text-based virtual world where multiple users can interact simultaneously in real-time. The software follows a traditional client-server architecture with the server handling all game logic, user management, and world state.

## High-Level Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   TCP Clients   │    │   TCP Clients   │    │   TCP Clients   │
│   (Telnet/etc)  │    │   (Telnet/etc)  │    │   (Telnet/etc)  │
└─────────┬───────┘    └─────────┬───────┘    └─────────┬───────┘
          │                      │                      │
          └──────────────────────┼──────────────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │    TinyMUSE Server        │
                    │   (Main Game Engine)      │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │     Database File         │
                    │  (Object Persistence)     │
                    └───────────────────────────┘
```

## Core System Components

### 1. Network Layer
- **File**: `io.bsd.c`
- **Purpose**: Handles TCP/IP connections using BSD sockets
- **Key Features**:
  - Multiple simultaneous client connections
  - Non-blocking I/O for responsive server performance
  - Connection management and cleanup
  - Client authentication and session management

### 2. Database System
- **Files**: `db.c`, `db.h`, `db.*.c`
- **Purpose**: Manages persistent game world data
- **Key Features**:
  - Object-oriented database with four main types: Rooms, Things, Exits, Players
  - Attribute system for object properties
  - Parent-child relationships and inheritance
  - Database versioning and migration support
  - Periodic auto-saves and manual database dumps

### 3. Command Processing System
- **Files**: `game.c`, `comm.*.c`
- **Purpose**: Parses and executes user commands
- **Key Features**:
  - Command parsing with token recognition
  - Command dispatch to appropriate handlers
  - Permission checking and validation
  - Recursive command execution prevention
  - Command logging and monitoring

### 4. User Management System
- **Files**: `players.c`, `class.c`, `powers.c`
- **Purpose**: Handles user authentication and authorization
- **Key Features**:
  - User classes (Guest, Visitor, Citizen, Builder, Admin, etc.)
  - Power-based permission system
  - Password management and encryption
  - User session tracking
  - Connection limits and access controls

### 5. Communication System
- **Files**: `comm.speech.c`, `comm.*.c`
- **Purpose**: Enables user-to-user communication
- **Key Features**:
  - Say/pose commands for local communication
  - Page system for private messaging
  - Channel-based chat system
  - Broadcasting capabilities
  - Message filtering and privacy controls

### 6. Object System
- **Files**: `db.c`, `move.c`, `match.c`
- **Purpose**: Manages game world objects and interactions
- **Key Features**:
  - Four primary object types: Room, Thing, Exit, Player
  - Object ownership and permissions
  - Location-based object containment
  - Object linking and relationships
  - Attribute-based object properties

### 7. Attribute System
- **Files**: `attrib.h`, `db.inherit.c`
- **Purpose**: Provides flexible object property system
- **Key Features**:
  - Typed attributes with various flags
  - Inheritance from parent objects
  - Permission-based attribute access
  - Built-in and user-defined attributes
  - Attribute-based scripting capabilities

## Data Flow Architecture

### Command Processing Flow
1. **Input Reception**: Network layer receives command from client
2. **Command Parsing**: Game engine parses command and arguments
3. **Permission Check**: System verifies user has permission to execute command
4. **Command Dispatch**: Appropriate command handler is called
5. **Database Operations**: Command handler reads/writes to database as needed
6. **Response Generation**: System generates appropriate response messages
7. **Output Transmission**: Network layer sends responses to relevant clients

### Database Operations Flow
1. **Object Access**: Commands access objects through database API
2. **Attribute Resolution**: System resolves attributes including inheritance
3. **Permission Validation**: Database checks object access permissions
4. **State Modification**: Changes are made to object state
5. **Change Propagation**: Updates are propagated to affected clients
6. **Persistence**: Changes are eventually written to disk

## Key Design Patterns

### 1. Command Pattern
- Each user command is implemented as a separate function
- Commands are dispatched through a centralized command processor
- Supports complex command chaining and scripting

### 2. Observer Pattern
- Objects can "listen" to events in their environment
- Communication system notifies all relevant players of events
- Attribute changes can trigger actions

### 3. Composite Pattern
- Objects can contain other objects (rooms contain things, players, exits)
- Hierarchical object relationships with inheritance
- Recursive operations on object trees

### 4. Strategy Pattern
- Different object types have different behaviors
- Attribute flags modify object behavior
- Pluggable permission and access control systems

## System Boundaries and Interfaces

### External Interfaces
- **Network Protocol**: Telnet-compatible text-based protocol
- **Database Format**: Custom text-based database format
- **Configuration**: Header-based compile-time configuration
- **Logging**: File-based logging system

### Internal Interfaces
- **Database API**: Object access and manipulation functions
- **Command Interface**: Standardized command function signatures
- **Permission System**: Power-based access control
- **Communication API**: Message routing and delivery

## Architectural Strengths

1. **Modularity**: Clear separation of concerns between components
2. **Extensibility**: Easy to add new commands and object types
3. **Scalability**: Efficient handling of multiple concurrent users
4. **Persistence**: Robust database system with backup capabilities
5. **Security**: Comprehensive permission and access control systems

## Architectural Limitations

1. **Single Process**: No horizontal scaling capabilities
2. **Text-Based Protocol**: Limited to character-based interfaces
3. **Synchronous Processing**: Commands block until completion
4. **Memory Management**: Manual memory management with potential leaks
5. **Platform Dependency**: Unix/Linux specific networking code

## Dependencies

- **System**: Unix/Linux operating system
- **Libraries**: Standard C library, BSD sockets, crypt library
- **Tools**: GCC compiler, make build system
- **Runtime**: File system access for database and logs

This architecture provides a solid foundation for a multi-user text-based virtual environment, with clear separation of concerns and extensible design patterns suitable for the era when it was developed.