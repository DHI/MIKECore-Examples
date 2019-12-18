using System;
using System.Collections.Generic;

namespace DHI.MikeCore.Examples
{
  public static class Extensions
  {
    //public static int FindIndex<T>(this IList<T> source, Predicate<T> match)
    public static int FindIndex<T>(this IList<T> source, Func<T, bool> match)
    {
      for (int i = 0; i < source.Count; i++)
      {
        if (match(source[i]))
        {
          return i;
        }
      }
      return -1;
    }
  }
}