// Soubor Kap13\06\Pokus.java
// Ukazuje pou�it� metody printf()
//
// Jen pro JDK 5
//

public class Pokus
{
  public static void main(String [] a)
  {
    double x = 3.14159;
    long l = Long.MIN_VALUE;
    System.out.printf("Zadan� ��sla jsou %f\n a %d%%\n", x, l);
    System.out.printf("%7.2f\n", x, l);
  }
}