#ifndef _Point_H
#define _Point_H

class Point {
public:
    Point(int x, int y);
    void addTo(Point other);

    int xValue, yValue;
};

#endif /* _Point_H */
