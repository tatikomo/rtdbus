BEGIN {
  begin_structs = 0;
  print "{\n  \"ACQINFOS\" : ["
}
END {
  print "  ]\n}"
}
/BEGIN/ {
  print "    {";
  begin_structs = 0;
  structs = "";
  struct = "";
}
/END/ {
  if (begin_structs == 1) {
    # Удалить последние два символа ',' и '\n' из structs
    print substr(structs, 0, length(structs)-2);
    print "\t]";
    begin_structs = 0;
  }
  print "    },";
}
/SYS_NAME\ =\ (\/.*)$/ { print "\t\"TAG\" : \""$3"\"," }
/TYPE\ =\ (.*)$/ {
  typ = $3
  if ($3 == "0") { typ="TS" }
  else if ($3 == "1") { typ="TM" }
  else if ($3 == "3") { typ="TSA" }
  else if ($3 == "4") { typ="TSC" }
  else if ($3 == "6") { typ="AL" }
  else if ($3 == "7") { typ="ICS" }
  else if ($3 == "8") { typ="ICM" }
  else if ($3 == "30") { typ="DIR" }
  else if ($3 == "31") { typ="DIPL" }
  else if ($3 == "50") { typ="SA" }
  else typ="UNK"

  print "\t\"TYPE\" : \""typ"\","
}
/IDENT\ =\ (.*)$/ { print "\t\"LED\" : "$3"," }
/CATEG\ =\ (.*)$/ { print "\t\"CATEGORY\" : \""$3"\"," }
/STRUCT\ =\ (.*)$/ {
  if (begin_structs == 0) {
    print "\t\"ASSOCIATES\" : [";
    begin_structs = 1;
  }
  struct = "\t\t{ \"STRUCT\" : \""$3"\" },\n";
  structs = structs struct;
}

