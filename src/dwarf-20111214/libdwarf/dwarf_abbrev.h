#ifndef DWARF_ABBREV_H
#define DWARF_ABBREV_H 1

extern int dwarf_get_abbrev(Dwarf_Debug dbg, Dwarf_Unsigned offset,
                            Dwarf_Abbrev *returned_abbrev,
                            Dwarf_Unsigned *length, Dwarf_Unsigned *abbr_count,
                            Dwarf_Error *error);

extern int dwarf_get_abbrev_code(Dwarf_Abbrev abbrev,
                                 Dwarf_Unsigned *returned_code,
                                 Dwarf_Error *error);

extern int dwarf_get_abbrev_tag(Dwarf_Abbrev abbrev, Dwarf_Half *returned_tag,
                                Dwarf_Error *error);

extern int dwarf_get_abbrev_children_flag(Dwarf_Abbrev abbrev,
                                          Dwarf_Signed *returned_flag,
                                          Dwarf_Error *error);

extern int dwarf_get_abbrev_entry(Dwarf_Abbrev abbrev, Dwarf_Signed index,
                                  Dwarf_Half *returned_attr_num,
                                  Dwarf_Signed *form, Dwarf_Off *offset,
                                  Dwarf_Error *error);

extern int dwarf_get_abbrev_count(Dwarf_Debug dbg);

#endif /* !DWARF_ABBREV_H */
