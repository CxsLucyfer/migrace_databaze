/* Soubor Kap06\06\SvStr.java
   Uk�zka deklarace v��tov�ho typu 
   a z�kladn�ch operac� s n�m

   JEN V JDK 5
 */

enum Strany {SEVER, JIH, VYCHOD, ZAPAD;}

public class SvStr
{
  public static void main(String[] arg)
  {
     Strany[] s = Strany.values();
     int i = 0;
     while(i < s.length)
     {
       System.out.print(s[i].name());
       if(s[i] == Strany.JIH) System.out.println("!");
       else System.out.println();
       i++;
     }
  }
}