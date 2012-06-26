// Soubor Kap09\06\Max.java
// P��klad metody, u n� p�edem nezn�me 
// po�et skute�n�ch parametr�.
//
// Metoda max() najde maximum z p�edem 
// neur�en�ho po�tu cel�ch ��sel

// �e�en� pro v�echny verze JDK: Pou�ijeme pole

public class Max
{
  public static int max(int[] x)
  {                              // res obsahuje budouc� v�sledek
    int res = Integer.MIN_VALUE; // Za��n�me nejmen�� mo�nou hodnotou
    if(x != null)                // Je-li parametr null, konec
    {                            // Projdi pole a najdi nejv�t�� prvek
      for(int i = 0; i < x.length; i++)
        if(x[i] > res) res = x[i];
    }
    return res;                  // a vra� ho
  }



  public static void main(String[] s)
  {
   int a=1, b=2, c=3;
   // Pou�it�: vytvo� ze zkouman�ch hodnot pole
   System.out.println(max(new int[]{a, b, c}));
  } 
}
