# TinyMUSE Features

This document provides a comprehensive list of features available in the TinyMUSE system, organized by functional area. These features represent the core capabilities that should be preserved in any modernization effort.

## Core Features Overview

TinyMUSE provides a rich set of features for creating and managing a text-based multi-user virtual environment. The system supports multiple user classes, complex object interactions, and sophisticated communication tools.

*Architecture Reference: See [Software Architecture](software-architecture.md) and [Server Architecture](server-architecture.md) for implementation details.*

## User Management Features

### Authentication and Access Control
- **User Registration**: New user account creation with password protection
- **Login System**: Secure user authentication using encrypted passwords
- **User Classes**: Hierarchical permission system (Guest → Visitor → Group → Citizen → PCitizen → Guide → Official → Builder → Admin → Director)
- **Session Management**: Multiple connections per user, session tracking
- **Access Control**: IP-based lockout system for banned users
- **Password Management**: User-controlled password changes, admin password reset

### User Classes and Permissions
- **Guest**: Limited access, cannot create objects
- **Visitor**: Basic exploration capabilities
- **Group**: Group membership functionality
- **Citizen**: Standard user privileges
- **PCitizen**: Premium citizen features
- **Guide**: Helper/mentor capabilities
- **Official**: Administrative assistance
- **Builder**: Object creation and modification
- **Admin**: Full system administration
- **Director**: Ultimate system control

### Power System
Fine-grained permission control with powers including:
- **POW_ANNOUNCE**: Server-wide announcements
- **POW_BOOT**: Disconnect other users
- **POW_BROADCAST**: Send messages to all users
- **POW_CHOWN**: Change object ownership
- **POW_CLASS**: Modify user classes
- **POW_DB**: Database administration
- **POW_EXAMINE**: Examine any object
- **POW_WHO**: See all online users
- **POW_MONEY**: Manipulate credits/currency
- **POW_SPOOF**: Impersonate other users
- *[And 40+ additional powers]*

## Object System Features

### Object Types
- **Rooms**: Locations where players can gather
- **Things**: Objects that can be picked up and manipulated
- **Exits**: Connections between rooms
- **Players**: User avatars in the virtual world

### Object Management
- **Creation**: `@create`, `@dig`, `@open` commands for object creation
- **Destruction**: `@destroy`, `@undestroy` for object lifecycle
- **Ownership**: Object ownership tracking and transfer
- **Inheritance**: Parent-child relationships between objects
- **Cloning**: `@clone` command for object duplication
- **Recycling**: Automatic cleanup of unused objects

### Object Properties
- **Attributes**: Flexible property system with 200+ built-in attributes
- **Flags**: Object behavior modifiers (Dark, Sticky, Haven, etc.)
- **Locks**: Access control for object interactions
- **Descriptions**: Multiple description types (desc, idesc, adesc, odesc)
- **Zone System**: Object organization and security zones

## Communication Features

### Local Communication
- **Say**: Speak to others in the same room (`"Hello everyone`)
- **Pose**: Perform actions visible to others (`:waves hello`)
- **Whisper**: Private communication to specific users
- **Emit**: Room-wide messages with custom text

### Remote Communication
- **Page**: Private messaging to users anywhere (`page user=message`)
- **Mail System**: Persistent messaging with inbox management
- **Channels**: Topic-based chat channels with aliases
- **Announcements**: Server-wide notifications

### Advanced Communication
- **Channel System**: Multiple chat channels with custom aliases
- **Channel Titles**: Personalized channel identification
- **Communication Filters**: Block unwanted communications
- **Away Messages**: Automatic responses when unavailable
- **Idle Tracking**: Monitor user activity levels

## Movement and Navigation

### Basic Movement
- **Go**: Move through exits (`go north`, `north`, `n`)
- **Teleport**: Instant travel (with appropriate permissions)
- **Home**: Return to designated home location
- **Follow**: Automatically follow another player

### Advanced Navigation
- **Exits**: Directional and named exits between rooms
- **Linking**: Connect rooms with exits
- **Jump**: Special movement for privileged users
- **Bearing**: Compass-based navigation
- **Sweep**: Search for objects and players

## Building and Creation

### World Building
- **Room Creation**: `@dig` command to create new rooms
- **Exit Creation**: `@open` command to create connections
- **Object Creation**: `@create` command for things and furniture
- **Linking**: Connect exits to destinations
- **Zone Management**: Organize areas with security zones

### Object Customization
- **Descriptions**: Set appearance and behavior text
- **Attributes**: Custom properties for objects
- **Locks**: Access control and security
- **Triggers**: Automated responses to events
- **Inheritance**: Share properties between objects

### Advanced Building
- **Parent Objects**: Template-based object creation
- **Attribute Inheritance**: Property sharing between objects
- **Zone Control**: Area-based permissions and organization
- **Quota System**: Limit object creation per user
- **Building Permissions**: Controlled access to creation tools

## Game Mechanics

### Economy System
- **Credits**: Virtual currency for transactions
- **Costs**: Object creation and maintenance costs
- **Payments**: Transfer credits between users
- **Quotas**: Limit resource usage per user
- **Deposits**: Object value and recycling

### Interaction Systems
- **Get/Drop**: Pick up and put down objects
- **Give**: Transfer objects between users
- **Use**: Interact with objects and devices
- **Enter/Leave**: Move in and out of vehicles or containers
- **Lock System**: Complex access control mechanisms

### Automation Features
- **Triggers**: Automated responses to events
- **Listeners**: Objects that react to speech
- **Queues**: Delayed command execution
- **Startup**: Automatic actions when objects are created
- **Periodic**: Recurring automated actions

## Administrative Features

### Database Management
- **Database Dumps**: Manual and automatic backups
- **Database Integrity**: Consistency checking and repair
- **Object Statistics**: Usage and performance metrics
- **Search Functions**: Find objects and users
- **Cleanup Tools**: Remove unused objects and data

### User Administration
- **User Creation**: Admin-created accounts
- **Password Management**: Reset user passwords
- **Class Management**: Assign user classes and permissions
- **Boot/Disconnect**: Remove problematic users
- **Site Lockouts**: IP-based access control

### System Monitoring
- **Statistics**: Server performance and usage statistics
- **Logging**: Comprehensive system and user activity logs
- **WHO Lists**: See all connected users
- **Queue Monitoring**: Track command execution
- **Resource Usage**: Monitor memory and CPU usage

## Communication and Social Features

### Social Systems
- **Aliases**: Custom names for users
- **Doing**: Status messages visible to others
- **Captions**: Brief descriptions for objects
- **Flags**: Social indicators and preferences
- **Privacy Controls**: Manage visibility and communications

### Community Features
- **Groups**: Organized user communities
- **Leadership**: Group management and delegation
- **Announcements**: Community-wide messaging
- **Help System**: Comprehensive online help
- **News System**: Server updates and information

## Technical Features

### Networking
- **Multiple Connections**: Support for simultaneous users
- **Concentrator Support**: Proxy connection handling
- **Ident Support**: User identification protocol
- **Connection Tracking**: Monitor and manage connections
- **Protocol Support**: Telnet-compatible text protocol

### Security
- **Password Encryption**: Secure password storage
- **Permission System**: Granular access control
- **Audit Logging**: Track administrative actions
- **Input Validation**: Protect against malicious input
- **Resource Limits**: Prevent abuse and overuse

### Performance
- **Efficient Database**: Optimized object storage
- **Memory Management**: Custom memory allocation
- **Caching**: Improved performance for frequent operations
- **Batch Processing**: Efficient bulk operations
- **Load Balancing**: Distribute processing load

## Extensibility Features

### Customization
- **Attribute System**: Flexible object properties
- **Function System**: User-defined functions
- **Command Extensions**: Custom command creation
- **Object Types**: Extensible object classification
- **Flag System**: Customizable object behaviors

### Programming Interface
- **Boolean Expressions**: Complex logical conditions
- **String Functions**: Text manipulation capabilities
- **Mathematical Functions**: Numeric calculations
- **List Processing**: Data structure manipulation
- **Pattern Matching**: Regular expression support

## Integration Features

### External Integration
- **File System**: Read/write external files
- **Email Integration**: Send notifications via email
- **Log Analysis**: External log processing
- **Database Export**: Export data for external use
- **Configuration Management**: External configuration files

### Compatibility
- **TinyMUD Compatibility**: Compatible with TinyMUD clients
- **Standard Protocols**: Telnet and text-based interfaces
- **Cross-Platform**: Unix/Linux system compatibility
- **Legacy Support**: Backward compatibility with older databases

## User Experience Features

### Convenience Features
- **Tab Completion**: Command and name completion
- **Command History**: Recall previous commands
- **Aliases**: Custom command shortcuts
- **Macros**: Complex command sequences
- **Shortcuts**: Single-character commands

### Accessibility
- **ANSI Color Support**: Colorized output
- **Screen Width**: Configurable display width
- **Terse Mode**: Reduced output for slow connections
- **Help System**: Comprehensive built-in help
- **Examples**: Built-in examples and tutorials

## Notable Feature Limitations

### Known Constraints
- **Single Server**: No horizontal scaling
- **Text Only**: No graphics or multimedia
- **No Encryption**: Plain text protocol
- **Limited Scripting**: Basic automation only
- **No Web Interface**: Telnet/terminal only

### Era-Specific Limitations
- **Memory Constraints**: Designed for limited RAM
- **Network Limitations**: Optimized for slow connections
- **Storage Limitations**: Text-based database only
- **Processing Limitations**: Single-threaded operation

## Modernization Opportunities

*Note: These are not current features but represent potential areas for enhancement:*

### Potential Enhancements
- **Web Interface**: RESTful API and web client
- **Mobile Support**: Mobile-friendly interfaces
- **Real-time Updates**: WebSocket-based communication
- **Rich Media**: Support for images, audio, video
- **Modern Authentication**: OAuth, 2FA, SSO
- **Encryption**: TLS/SSL security
- **Horizontal Scaling**: Multi-server architecture
- **Database Modernization**: SQL or NoSQL backends

This comprehensive feature set represents decades of development in text-based virtual environments and provides a solid foundation for understanding the full capabilities of the TinyMUSE system.