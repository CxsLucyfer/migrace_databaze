// Soubor Kap12\02\Pomoc.java
// Uk�zka deklarace parametrizovan�ch metod
// Jen JDK 5


public class Pomoc
{
  // Najde prvek le��c� uprost�ed pole typu T
  public static <T> T median(T[] x)
  {
    if(x == null || x.length == 0) throw new IllegalArgumentException(); 
    return x[x.length / 2];
  }
  public static <T> void swap(int i, int j, T[] x)
  {
   T a = x[i];
   x[i] = x[j];
   x[j] =a;
  } 

  // Vypo�te pr�m�r z prvk� v dan�m poli
  public static <T extends Comparable> void sort(T[] x)
  {
    for(int i = 0; i < x.length; i++)
    {
      int min = i;
      for(int j = i+1; j < x.length; j++)
         if(x[j].compareTo(x[min]) < 0) min = j;  // Najdi index nejmen��ho
      if(min != i) swap(min, i, x);  // a proho� ho s prvn�m v dan�m �seku
    } 
  }
}