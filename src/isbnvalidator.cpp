/***************************************************************************
    copyright            : (C) 2002-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "isbnvalidator.h"

#include <kdebug.h>

using Tellico::ISBNValidator;

ISBNValidator::ISBNValidator(QObject* parent_, const char* name_/*=0*/)
    : QValidator(parent_, name_) {
}

QValidator::State ISBNValidator::validate(QString& input_, int& pos_) const {
  // first check to see if it's a "perfect" ISBN
  // A perfect ISBN has 9 digits plus either an 'X' or another digit
  // A perfect ISBN may have 2 or 3 hyphens
  // The final digit or 'X' is the correct check sum
  static const QRegExp isbn(QString::fromLatin1("(\\d-?){9,11}-[\\dX]"));
  unsigned len = input_.length();
/*
  // Don't do this since the hyphens may be in the wrong place, can't put that in a regexp
  if(isbn.exactMatch(input_) // put the exactMatch() first since I use matchedLength() later
     && (len == 12 || len == 13)
     && input_[len-1] == checkSum(input_)) {
    return QValidator::Acceptable;
  }
*/
  // two easy invalid cases are too many hyphens and the 'X' not in the last position
  if(input_.contains('-') > 3
     || input_.contains('X', false) > 1
     || (input_.find('X', 0, false) != -1 && input_[len-1].upper() != 'X')) {
    return QValidator::Invalid;
  }

  // remember if the cursor is at the end
  bool atEnd = (pos_ == static_cast<int>(len));

  // fix the case where the user attempts to delete a character from a non-checksum
  // position; the solution is to delete the checksum, but only if it's X
  if(!atEnd && input_[len-1].upper() == 'X') {
    input_.truncate(len-1);
    --len;
  }

  // fix the case where the user attempts to delete the checksum; the
  // solution is to delete the last digit as well
  static const QRegExp digit(QString::fromLatin1("\\d"));
  if(atEnd && input_.contains(digit) == 9 && input_[len-1] == '-') {
    input_.truncate(len-2);
    pos_ -= 2;
    len -= 2;
  }

  // now fixup the hyphens and maybe add a checksum
  fixup(input_);
  len = input_.length(); // might have changed in fixup()
  if(atEnd) {
    pos_ = len;
  }

  if(isbn.exactMatch(input_) && (len == 12 || len == 13)) {
    return QValidator::Acceptable;
  } else {
    return QValidator::Intermediate;
  }
}

void ISBNValidator::fixup(QString& input_) const {
  if(input_.isEmpty()) {
    return;
  }

  //replace "x" with "X"
  input_.replace('x', QString::fromLatin1("X"));

  // remove invalid chars
  static const QRegExp badChars(QString::fromLatin1("[^\\d-X]"));
  input_.remove(badChars);

  // special case for EAN values that start with 978 or 979. That's the case
  // for things like barcode readers that essentially 'type' the string at
  // once. The simulated typing has already caused the input to be normalized,
  // so strip that off, as well as the generated checksum. Then continue as normal.
  //  If someone were to input a regular 978- or 979- ISBN _including_ the
  // checksum, it will be regarded as barcode input and the input will be stripped accordingly.
  // I consider the likelihood that someone wants to input an EAN to be higher than someone
  // using a Nigerian ISBN and not noticing that the checksum gets added automatically.
  if(input_.length() > 12
     && (input_.startsWith(QString::fromLatin1("978")))
         || input_.startsWith(QString::fromLatin1("979"))) {
     // Strip the first 4 characters (the invalid publisher)
     input_ = input_.right(input_.length() - 3);
  }

  // hyphen placement for some languages publishers is well-defined
  // remove all hyphens, and insert them ourselves
  // some countries have ill-defined second hyphen positions, and if
  // the user inserts one, then be sure to put it back

  // Find the first hyphen. If there is none,
  // input_.find('-') returns -1 and hyphen2_position = 0
  unsigned hyphen2_position = input_.find('-') + 1;

  // Find the second one. If none, hyphen2_position = -2
  hyphen2_position = input_.find('-', hyphen2_position) - 1;

  // The second hyphen can not be in the last characters
  if(hyphen2_position >= 9) {
    hyphen2_position = 0;
  }

  // Remove all existing hyphens. We will insert ours.
  input_.remove('-');
  // the only place that 'X' can be is last spot
  for(int xpos = input_.find('X'); xpos > -1; xpos = input_.find('X', xpos+1)) {
    if(xpos < 9) { // remove if not 10th char
      input_.remove(xpos, 1);
      --xpos;
    }
  }
  input_.truncate(10);

  // If we can find it, add the checksum
  if(input_.length() > 8) {
    input_[9] = checkSum(input_);
  }

  unsigned long range = input_.leftJustify(9, '0', true).toULong();

  // now find which band the range falls in
  unsigned band = 0;
  while(range >= bands[band].MaxValue) {
    ++band;
  }

  // if we have space to put the first hyphen, do it
  if(input_.length() > bands[band].First) {
    input_.insert(bands[band].First, '-');
  }

  //add 1 since one "-" has already been inserted
  if(bands[band].Mid != 0) {
    hyphen2_position = bands[band].Mid;
    if(input_.length() > (hyphen2_position + 1)) {
      input_.insert(hyphen2_position + 1, '-');
    }
  // or put back user's hyphen
  } else if((hyphen2_position > 0) && (input_.length() >= (hyphen2_position + 1))) {
    input_.insert(hyphen2_position + 1, '-');
  }

  // add a "-" before the checkdigit and another one if the middle "-" exists
  unsigned trueLast = bands[band].Last + 1 + (hyphen2_position > 0 ? 1 : 0);
  if(input_.length() > trueLast) {
    input_.insert(trueLast, '-');
  }
}

QChar ISBNValidator::checkSum(const QString& input_) const {
  unsigned sum = 0;
  unsigned multiplier = 10;

  // hyphens are already gone, only use first nine digits
  for(unsigned i = 0; i < input_.length() && multiplier > 1; ++i) {
    if(input_[i].isDigit()) {
      sum += input_[i].digitValue() * multiplier--;
    }
  }
  sum %= 11;
  sum = 11-sum;

  QChar c;
  if(sum == 10) {
    c = 'X';
  } else if(sum == 11) {
    c = '0';
  } else {
    c = QString::number(sum)[0];
  }
  return c;
}

// ISBN code from Regis Boudin
#define ISBNGRP_1DIGIT(digit, max, middle, last)        \
          {((digit)*100000000) + (max), 1, middle, last}
#define ISBNGRP_2DIGIT(digit, max, middle, last)        \
          {((digit)*10000000) + ((max)/10), 2, middle, last}
#define ISBNGRP_3DIGIT(digit, max, middle, last)        \
          {((digit)*1000000) + ((max)/100), 3, middle, last}
#define ISBNGRP_4DIGIT(digit, max, middle, last)        \
          {((digit)*100000) + ((max)/1000), 4, middle, last}
#define ISBNGRP_5DIGIT(digit, max, middle, last)        \
          {((digit)*10000) + ((max)/10000), 5, middle, last}

#define ISBNPUB_2DIGIT(grp) (((grp)+1)*1000000)
#define ISBNPUB_3DIGIT(grp) (((grp)+1)*100000)
#define ISBNPUB_4DIGIT(grp) (((grp)+1)*10000)
#define ISBNPUB_5DIGIT(grp) (((grp)+1)*1000)
#define ISBNPUB_6DIGIT(grp) (((grp)+1)*100)
#define ISBNPUB_7DIGIT(grp) (((grp)+1)*10)
#define ISBNPUB_8DIGIT(grp) (((grp)+1)*1)

// how to format an ISBN, after categorising it into a range of numbers.
struct ISBNValidator::isbn_band ISBNValidator::bands[] = {
  /* Groups 0 & 1 : English */
  ISBNGRP_1DIGIT(0,     ISBNPUB_2DIGIT(19),      3, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_3DIGIT(699),     4, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_4DIGIT(8499),    5, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_5DIGIT(89999),   6, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_6DIGIT(949999),  7, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_7DIGIT(9999999), 8, 9),

  ISBNGRP_1DIGIT(1,     ISBNPUB_5DIGIT(54999),   6, 9),
  ISBNGRP_1DIGIT(1,     ISBNPUB_5DIGIT(86979),   6, 9),
  ISBNGRP_1DIGIT(1,     ISBNPUB_6DIGIT(998999),  7, 9),
  ISBNGRP_1DIGIT(1,     ISBNPUB_7DIGIT(9999999), 8, 9),
  /* Group 2 : French */
  ISBNGRP_1DIGIT(2,     ISBNPUB_2DIGIT(19),      3, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_3DIGIT(349),     4, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_5DIGIT(39999),   6, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_3DIGIT(699),     4, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_4DIGIT(8399),    5, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_5DIGIT(89999),   6, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_6DIGIT(949999),  7, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_7DIGIT(9999999), 8, 9),

  ISBNGRP_1DIGIT(7,     ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_2DIGIT(94,    ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_3DIGIT(993,   ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_4DIGIT(9989,  ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_5DIGIT(99999, ISBNPUB_2DIGIT(99),      0, 9)
};
