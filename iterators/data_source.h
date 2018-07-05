#pragma once

/*
    Data source, the same thing as input or forward iterator but implemented using abstract methods
    so pointer to implementation can be used without making it a template (simplifies some things).
    Called in this way to distinguish from C++ iterators (althouth actual implementations
    can implement both interfaces, and even encouraged to do it), just another way to do the same thing.

    Template parameter T defined a type of a value pointed to by this data source.
*/
template<typename T>
class DataSource
{
public:
    virtual T Get() = 0;
    virtual void Increment() = 0;
};