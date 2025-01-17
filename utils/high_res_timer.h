#pragma once

struct large_int
{
    large_int(long val=0)
    {
        _value.HighPart = 0;
        _value.LowPart  = val;
    }

    operator LARGE_INTEGER () const {return _value;}
    LARGE_INTEGER* operator&()      {return &_value;}
    LARGE_INTEGER _value;
};

class  high_res_timer
{
public:
     high_res_timer()
         : _last_point(0)
     {
        ::QueryPerformanceFrequency(&_frequency);
        set_point();
     }
    
     double set_point() 
     {   
         large_int point;
         ::QueryPerformanceCounter(&point);
         double interval = static_cast<double>(point._value.QuadPart - _last_point._value.QuadPart) / _frequency._value.QuadPart;
         _last_point = point;
         return interval;
     }

     double get_delta() 
     {   
         large_int point;
         ::QueryPerformanceCounter(&point);
         return static_cast<double>(point._value.QuadPart - _last_point._value.QuadPart) / _frequency._value.QuadPart;
     }

private:
     large_int     _frequency;
     large_int     _last_point;
};