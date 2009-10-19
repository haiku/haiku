/****************************************************************
 *    NAME: Michael J. Radwin
 *    ACCT: mradwin
 *    FILE: Rational.H
 *    DATE: Sat Mar  4 01:47:16 EST 1995
 *     $Id: Rational.H 10 2002-07-09 12:24:59Z ejakowatz $
 ****************************************************************/

class Rational {
  friend ostream & operator<<(ostream &output, const Rational &r);
  friend Rational pow(const Rational &r, int exp);
  friend Rational operator+(int n, const Rational &r);
  friend Rational operator-(int n, const Rational &r);
  friend Rational operator*(int n, const Rational &r);
  friend Rational operator/(int n, const Rational &r);
  friend int operator==(int n, const Rational &r);
  friend int operator!=(int n, const Rational &r);
  friend int operator< (int n, const Rational &r);
  friend int operator> (int n, const Rational &r);
  friend int operator<=(int n, const Rational &r);
  friend int operator>=(int n, const Rational &r);

public:
  Rational(int num, int den = 1);
  Rational(const Rational &r);
  ~Rational();

  Rational operator+(const Rational &r) const;
  Rational operator-(const Rational &r) const;
  Rational operator*(const Rational &r) const;
  Rational operator/(const Rational &r) const;
  const Rational & operator= (const Rational &r);
  const Rational & operator+=(const Rational &r);
  const Rational & operator-=(const Rational &r);
  const Rational & operator*=(const Rational &r);
  const Rational & operator/=(const Rational &r);

  Rational operator+(int r) const;
  Rational operator-(int r) const;
  Rational operator*(int r) const;
  Rational operator/(int r) const;
  const Rational & operator= (int r);
  const Rational & operator+=(int r);
  const Rational & operator-=(int r);
  const Rational & operator*=(int r);
  const Rational & operator/=(int r);

  Rational operator-() const;
  int operator! () const;
  int operator==(const Rational &r) const;
  int operator==(int r) const;
  int operator!=(const Rational &r) const;
  int operator!=(int r) const;
  int operator< (const Rational &r) const;
  int operator< (int r) const;
  int operator> (const Rational &r) const;
  int operator> (int r) const;
  int operator<=(const Rational &r) const;
  int operator<=(int r) const;
  int operator>=(const Rational &r) const;
  int operator>=(int r) const;

private:
  Rational() {}
  static int compute_gcd(int a, int b);
  static int gcd(int bigger, int smaller);
  int n, d;
};

// global prototypes
ostream & operator<<(ostream &output, const Rational &r);
Rational pow(const Rational &r, int exp);
Rational operator+(int n, const Rational &r);
Rational operator-(int n, const Rational &r);
Rational operator*(int n, const Rational &r);
Rational operator/(int n, const Rational &r);
int operator==(int n, const Rational &r);
int operator!=(int n, const Rational &r);
int operator< (int n, const Rational &r);
int operator> (int n, const Rational &r);
int operator<=(int n, const Rational &r);
int operator>=(int n, const Rational &r);
