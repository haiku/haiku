BeIDE .prof project file format - a reverse engineering attempt

This is based on Philippe Houdoin's previous work.

Hierarchical tags format.
Tag structure:

```
uint32  tag
uint32  tag data size
uint8   data[tag_data_size]

(Values are big endian - always?)

'MIDE'                                 = Metrowerks IDE top tag
       uint32                          = sizeof of tag data
       'DPrf'                          = Data project file? Define Prefs?
               uint32                  = size of tag data
               'PrEn'                  = Project/prefs encoding?
                       uint32          = size of tag data (4)
                       uint32          0xffffffff = ???
               'PrVr'                  = Project/pref Version?
                       uint32          = size of tag data (4)
                       uint32          0x00000003 = ???
               'Priv'                  = ???
                       uint32          = size of tag data (8)
                       uint32          0x00000007 = ???
                       uint32          0x00000000 = ???
               'DAcc'                  = ???
                       uint32          = size of tag data (16 = 0x10)
                       uint32          0x00000006 = ???
                       uint32          0x00000000 = ???
                       uint32          0x00000005 = ???
                       uint32          0x00000002 = ??? (flags fields)
               'SPth'                  = Search Paths tag (fixed tag size of 0x0108 = 264 bytes)
                       uint32          = size of tag data (264 = 0x108)
                       uint32          = Flags field ? (values seen so far: 0, 1 or 3)
                       uint8           = boolean ??? (local or system? precompiled/cached?)
                       uint8[259]      = path string
               'PPth'                  = Project paths (reuse the same tag format above, see SPth tag)
               'Trgg'                  = Table of build "Trigger" ? Tag data is n x 140 bytes
record of bellow format:
                       uint32          = size of tag data (n x 140 bytes)
                               uint16          = build stage ? (ranging from 0-never, 1-preproc,
                                                                2-compile, 3-link, 4-resource, ?=always)
                               uint16          = build flags (0 ou 1 ou 4 + 2 + 1 = ?)
                               uint8[8]        = file kind extension
                               uint8[64]       = tool kind (gcc_link, gcc, rsrc ?)
                               uint8[64]       = file kind mime type
               'GPrf'                  = Global prefs ?
                       uint32          = size of tag data
                       'GenB'          = Generic block ?
                               uint32          = size of tag data
                               one single generic_block

                               generic_block has the following structure:
                                       uint32  = block group ID
                                       uint32  = value size
                                       string with zero ending = value name
                                       uint32  = flags (1 = factory/default value?, 7 = custom ? To investigate)
                                       uint8[value size - 4] = value

                               List of generic blocks groups and names found so far:

                               'MWPr'  = Metrowerks prefs ?
                                       name = 'AppEditorPrefs', value = binary blob with colors, booleans, flags
                               'cccg'  = C compiler config
                                       name = 'gccCompilerOptions', value = gcc options string
                                       name = 'gccLanguage', value = language number ?
                                       name = 'gccCommonWarning', value = array of uint8 booleans ?
                                       name = 'gccWarning', value = array of uint8 booleans ?
                                       name = 'gccCodeGeneration', value = array of uint8 booleans ?
                               'dlcg'  = dynamic linker config
                                       name = 'gccLinkerOptions', value = gcc linker options string
                                       name = 'AdditionalGCCLinkerOptions', value = additions linker options string
                               'GNOL' = ??? (Globally needs object linking ?)
                                       name = 'UpdateProject, value = uint32 (boolean?)
                               'mwcx' = Metrowerks C++ ?
                                       name = 'ProjectPrefsx86', value = binary blob with:
                                               uint8[9] of flags & like...
                                               uint8[64] = output file mime type (for setmime) (usually
                                                           'application/x-vnd.Be-elfexecutable'!)
                                               uint8[67 (!?)] = output file name
                                               -> To investigate deeper
                               'RsDt' = Resource Data config
                                       name = 'ResData', value = array of uint8 booleans ?
                               'mwcc' = Metrowerks C compiler
                                       name = 'ProcessorPrefs', value = binary blob/array of booleans
                                       name = 'XLanguagePrefs', value = binary blob/array of booleans
                                       name = 'WarningsPrefs', value = binary blob/array of booleans
                                       name = 'DisassemblerPrefs', value = ???
                                       name = 'GlobalOpts', value = ???
                               'mwld' = Metrowerks linker
                                       name = 'LinkerPrefs', value = few array of booleans + 3 x
                                               uint8[64), the symbol names of the
                                               __start, _term_routine and _init_routine routines.
                                       name = 'PEFPrefs', value = we don't care about PEF ;-)
                                       name = 'ProjectNamePrefs', value =
                                                       uint8[64] = Default output name string ('Application')
                                                       SUBTYPE in big endian ('????')
                                                       TYPE 'BAPP' in big endian
                                                       uint8[64] = output mime type string
                                                       (application/x-be-executable)
                               'ShDt' = Shell Data
                                       name = 'ShellData', value = shell string options (usually '-noprofile')
                               'msan' = NASM config
                                       name = 'NASMOptions', value = cryptic binary blob :-(

       'Sect' = Section (Group section in project list)
               uint32 = size of tag data (always 0x50 bytes long)
               uint32 = nb items ('Link', 'PLnk', 'Fil1' or 'IgFl' tags) in this section
               uint32[2] = flags ?
               uint8 = ??? boolean ???
               uint8[*] = Name of the section

       'Link' = Link file
               uint32 = size of tag data
               uint32[6] = ???
               One or more file (sub-)tag:

               List of file sub-tags found so far:
                       'Mime' = value = MIME type of the file (one per parent tag)
                               uint32 = sizeof tag data
                               uint32 = size of value = size of tag data - sizeof(uint32)
                               uint8[size of value] = MIME type of the file
                       'MSFl' = Master/Main/Module? Source File (one per parent tag)
(either a library file, or a source file)
                               uint32 = sizeof tag data
                               uint32[2] = flags (to investigate)
                               'Name'
                               uint32 = sizeof 'Name' tag
                               uint32 = sizeof 'Name' value
                               uint8[size of 'Name' value] = File name (no path)
                       'SrFl' = Source File dependencies ([0..n] tags after the 'MSFl' tag) (mostly, headers - presents in Fil1 parent tag)
                               uint32 = sizeof tag data
                               uint32[2] = flags (to investigate)
                               'Name'
                               uint32 = sizeof 'Name' tag
                               uint32 = sizeof 'Name' value
                               uint8[size of 'Name' value] = File name (no path)

       'PLnk' = Post-Link file? (e.g. resource file)
               Same structure as 'Link'
       'Fil1' = One build file
               Same structure as 'Link'
       'IgFl' = Ignored File, for file user has added to project list for easy access but is not build (e.g. local headers)
               Same structure as 'Link'. Note that the next 2 following uint32 (off 6) are then set to 0xFFFF each.
       'Brws' = Source browser info
               uint32 = size of tag
               uint8[*]
```

More info can be found in proj2make source code at https://git.haiku-os.org/haiku/tree/3rdparty/proj2make.
