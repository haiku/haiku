#! /usr/bin/perl -w
open F, "cat C-translit.h.in | gcc -E - |" || die "Cannot preprocess input file";


sub cstrlen {
  my($str) = @_;
  my($len) = length($str);
  my($cnt);
  my($res) = 0;

  for ($cnt = 0; $cnt < $len; ++$cnt) {
    if (substr($str, $cnt, 1) eq '\\') {
      # Recognize the escape sequence.
      if (substr($str, $cnt + 1, 1) eq 'x') {
	my($inner);
	for ($inner = $cnt + 2; $inner < $len && $inner < $cnt + 10; ++$inner) {
	  my($ch) = substr($str, $inner, 1);
	  next if (($ch ge '0' && $ch le '9')
		   || ($ch ge 'a' && $ch le 'f')
		   || ($ch ge 'A' && $ch le 'F'));
	  last;
	}
	$cnt = $inner;
	++$res;
      } else {
	die "invalid input" if ($cnt + 1 >= $len);
	++$res;
	++$cnt;
      }
    } else {
      ++$res;
    }
  }

  return $res;
}

while (<F>) {
  next if (/^#/);
  next if (/^[ 	]*$/);
  chop;

  if (/"([^\"]*)"[ 	]*"(.*)"/) {
    my($from) = $1;
    my($to) = $2;
    my($fromlen) = cstrlen($from);
    my($tolen) = cstrlen($to);

    push(@froms, $from);
    push(@fromlens, $fromlen);
    push(@tos, $to);
    push(@tolens, $tolen);
  }
}

printf "#define NTRANSLIT %d\n", $#froms + 1;

printf "static const uint32_t translit_from_idx[] =\n{\n  ";
$col = 2;
$total = 0;
for ($cnt = 0; $cnt <= $#fromlens; ++$cnt) {
  if ($cnt != 0) {
    if ($col + 7 >= 79) {
      printf(",\n  ");
      $col = 2;
    } else {
      printf(", ");
      $col += 2;
    }
  }
  printf("%4d", $total);
  $total += $fromlens[$cnt] + 1;
  $col += 4;
}
printf("\n};\n");

printf "static const wchar_t translit_from_tbl[] =\n ";
$col = 1;
for ($cnt = 0; $cnt <= $#froms; ++$cnt) {
  if ($cnt != 0) {
    if ($col + 6 >= 79) {
      printf("\n ");
      $col = 1;
    }
    printf(" L\"\\0\"");
    $col += 6;
  }
  if ($col > 2 && $col + length($froms[$cnt]) + 4 >= 79) {
    printf("\n  ");
    $col = 2;
  } else {
    printf(" ");
    ++$col;
  }
  printf("L\"$froms[$cnt]\"");
  $col += length($froms[$cnt]) + 3;
}
printf(";\n");

printf "static const uint32_t translit_to_idx[] =\n{\n  ";
$col = 2;
$total = 0;
for ($cnt = 0; $cnt <= $#tolens; ++$cnt) {
  if ($cnt != 0) {
    if ($col + 7 >= 79) {
      printf(",\n  ");
      $col = 2;
    } else {
      printf(", ");
      $col += 2;
    }
  }
  printf("%4d", $total);
  $total += $tolens[$cnt] + 2;
  $col += 4;
}
printf("\n};\n");

printf "static const wchar_t translit_to_tbl[] =\n ";
$col = 1;
for ($cnt = 0; $cnt <= $#tos; ++$cnt) {
  if ($cnt != 0) {
    if ($col + 6 >= 79) {
      printf("\n ");
      $col = 1;
    }
    printf(" L\"\\0\"");
    $col += 6;
  }
  if ($col > 2 && $col + length($tos[$cnt]) + 6 >= 79) {
    printf("\n  ");
    $col = 2;
  } else {
    printf(" ");
    ++$col;
  }
  printf("%s", "L\"$tos[$cnt]\\0\"");
  $col += length($tos[$cnt]) + 5;
}
printf(";\n");

exit 0;
