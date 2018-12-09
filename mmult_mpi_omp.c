#include <stdio.h>
#include "mpi.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#define min(x, y) ((x)<(y)?(x):(y))

//Program to do matrix multiplication
//Sean McNamara and Adam Freedman

int main(int argc, char* argv[]){
  //Use i and j as numbers when we need them
  int i,j;
  //Basically a boolean almost to tell us whether we can or cannot multiply the two
  //matrices together
  bool compatable;
  int nrows, ncols;
  int *size;
  size=(int*)malloc(sizeof(int) * 3);
  double  *b, **answer;
  //Iteration is basically for us to use when we do vector multiplication with matrixes it will
  //tell us what col we are in during the multiplication
  int iteration;
  //This stuff is stuff that we got from the slides its basically just stuff that we will
  //use to help us comminicate with the slaves and some other jazz...
  double *buffer, ans;
  int run_index;
  int nruns;
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  int  numsent, sender;
  int anstype, row;
  srand(time(0));
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  //Check to see if we are using the a.txt and b.txt files (which we kinda have to)
  if(argc>2){
    //We will read from the files to create the arrays to represent matrices
    if(myid==0){

        FILE * file_one;
        FILE * file_two;
        int row_one, col_one, row_two, col_two;
        double *matrix_one,*matrix_two;

        //do an fopen on the first file and make it to read only
        file_one=fopen(argv[1],"r");

        //Check to make sure there are no errors opening the file and if there
        //are we just exit(0)
        if(file_one == NULL){
          printf("\nERROR ERROR ERROR ERROR ABORT ABORT ABORT (no file)\n");
          exit(0);
        }


        char line[128];

        // gets row and col values for first file
        if(fgets(line, sizeof line, file_one)!=NULL){
          // gets first line and parses to get row num
          // Code commented out below because its too many lines of code when it can be done in 4 (see above)
          char *ret;
          ret = strstr(line,"rows(");
          char *num;
          num = strchr(ret,')');
          int pos;
          pos = (int)(num - ret);
          //duplicates specific part of string and returns null pointer to the given spot
          num =strndup(ret+5,pos-5);
          //Convert the char that we got into the int value
          row_one = atoi(num);

          //Gets column num
          ret=strstr(ret,"cols(");
          num=strchr(ret,')');
          pos=(int)(num-ret);
          //duplicates specific part of string and returns null pointer to the given spot
          num= strndup(ret+5,pos-5);
          //convert char to int again for cols
          col_one=atoi(num);

        }

        //need to allocate memory and put the stuff from file_one into the matrix
        matrix_one = (double*)malloc(sizeof(double) * row_one * col_one);
        for(i=0; i < row_one; i++){
          for(j=0; j < col_one; j++){
            fscanf(file_one, "%lf" , &matrix_one[i * col_one + j]);
          }
        }
        //close the file now and move on to file two
        fclose(file_one);

        //open file two now and read from it, it should also be the second arg
        //basically in our case it is b.txt
        file_two = fopen(argv[2],"r");
        //check again to make sure that the file exists and if not we gotta exit
        if(file_two == NULL){
          printf("\nExiting because the file does not exist\n");
          exit(0);
        }

        //we get the values of row two and col two and put it into our second
        //matrix
        if(fgets(line,sizeof line, file_two)!=NULL){

          //We parse our first line because we want to figure out how many rows
          //we will have for this matrix
          char *ret = strstr(line,"rows(");
          char *num= strchr(ret,')');
          int pos = (int)(num-ret);
          //duplicates specific part of string and returns null pointer to the given spot
          num=strndup(ret+5,pos-5);
          //now that we got the given number in the parenthesis, we can use atoi to convert
          //that to an int
          row_two=atoi(num);

          //Do the same thing for the columns now to figure out how many columns
          //we will need for our matrix
          ret=strstr(ret,"cols(");
          num=strchr(ret,')');
          pos=(int)(num-ret);
          //duplicate specific part again
          num= strndup(ret+5,pos-5);
          //revert that char to an int
          col_two=atoi(num);
        }


        //Make sure to allocate that memory!
        matrix_two=(double*)malloc(sizeof(double) * row_two * col_two);
        //Now we put the values from the files into our matrix
        for(i=0;i<row_two;i++) {
         	for(j=0;j<col_two;j++){
             //do an fscanf to put the values from b.txt into our matrix two
             fscanf(file_two, "%lf", &matrix_two[i * col_two + j]);
          }
        }
        //close file two at the end to stop reading
        fclose(file_two);


        //We print out the two matrixes just to see that we did everything correctly
        //and so other users can see
        printf("Matrix One From a.txt file\n");
        printf("rows(%d),cols(%d)\n", row_one, col_one);
        for(i = 0; i < row_one ; i++){
          for(j = 0; j < col_one; j++){
            printf("%f ",matrix_one[i * col_one + j]);
	  }
	      printf("\n");
        }
        //Now print out the second matrix that we got from b.txt
        printf("\nMatrix Two From b.txt file \n");
        printf("rows(%d),cols(%d)\n", row_two, col_two);
        for(i = 0; i < row_two; i++){
          for(j = 0; j < col_two; j++){
            printf("%f ",matrix_two[i * col_two + j]);
       	  }
	      printf("\n");
        }

        //One of the rules of multiplying matrices is to make sure that the number of
        //columns in the first matrix is the same as the number of rows in the
        //second matrix. So here we need to check that, and set it to a boolean
        //so we know whether or not we can go through with the matrix multiplication.
        //Set compatable to true if they do match.
        if(col_one == row_two)
                compatable = true;
        //Set compatable to false if they don't match.
        else
                compatable = false;

        //If they are compatable we will then set some variable values to allow us to do
        //the multiplication, the code we use below is also something that we got from the slides
        //so thats why we are using the ncols and and nrows stuff
        if(compatable){
          ncols = col_one;
          nrows = row_one;
          size[0] = nrows;
          size[1] = ncols;
          size[2] = col_two;

          //Now we send the slaves info about the rows and cols
          for(i = 0; i < numprocs - 1; i++){
            //MPI_Send(buffer, count, datatype, dest, tag, MPI_COMM_WORLD);
            MPI_Send(size, 3, MPI_INT, i+1, i+1, MPI_COMM_WORLD);
          }

          //We need to allocate memory for these guys based on either the ncols
          //or the rows from matrix one
          b = (double*)malloc(ncols* sizeof(double));
          answer = (double **)malloc(row_one * sizeof(double *));

          //Need to do another memory allocation for this double with the second
          //columns
          for ( i = 0; i < row_one; i++){
            answer[i] = (double *)malloc(col_two * sizeof(double));
          }

          //Give memory allocation for a buffer
          buffer = (double*)malloc(ncols* sizeof(double));

          //Basically the way we solve this is by splitting the cols of second matrix
          //into vectors and then we do simple vector - matrix multiplication on them
          //to get our answer, iterations is basically used to iterate through the different cols
          starttime = MPI_Wtime();
          for(iteration = 0; iteration < col_two; iteration++) {
           //Our b valu is the vector (or a col of the second matrix)
            for(i = 0; i < ncols; i++){
              b[i] = matrix_two[i * col_two + iteration];
            }

            numsent = 0;
            MPI_Bcast(b, ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            //This is code that we got from the slide again (slide 69)
            for (i = 0; i < min(numprocs-1, nrows); i++) {
              for (j = 0; j < ncols; j++) {
                buffer[j] = matrix_one[i * ncols + j];
              }
              MPI_Send(buffer, ncols, MPI_DOUBLE, i+1, i+1, MPI_COMM_WORLD);
              numsent++;
            }

            //This is also code the we got from the slides (slide 70)
            for (i = 0; i < nrows; i++) {
              MPI_Recv(&ans, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG,
                      MPI_COMM_WORLD, &status);
              sender = status.MPI_SOURCE;
              anstype = status.MPI_TAG;
              answer[anstype-1][iteration] = ans;

              if (numsent < nrows) {
                for (j = 0; j < ncols; j++) {
                  buffer[j] = matrix_one[numsent*ncols + j];
                }
                MPI_Send(buffer, ncols, MPI_DOUBLE, sender, numsent+1,
                          MPI_COMM_WORLD);
                numsent++;
              }else {
                MPI_Send(MPI_BOTTOM, 0, MPI_DOUBLE, sender, 0, MPI_COMM_WORLD);
              }

            }
          }

          endtime= MPI_Wtime();
          printf("%f\n",(endtime - starttime));
          printf("\n");

          //This is just to show the user that we finished multiplying and where
          //we are storing the final answer of the multiplication to view
          printf("Yeah we finished multiplying! Your answer is in final.txt\n");
          for(i = 0; i < row_one; i++){
            for(j = 0; j < col_two; j++){
              printf("%f ", answer[i][j]);
            }
            printf("\n");
          }

          //Here is where we actually write back to the final.txt file so we can
          //store the final answer
          FILE *final;
          //Set the permission to write only
          final = fopen("final.txt","w");
          //We print back in the same format that was in
          //a.txt and b.txt
          fprintf(final, "rows(%d) cols(%d)\n", row_one, col_two);
          //Iterate through the final answer matrix to print out the values
          for(i = 0 ; i < row_one; i++) {
            for(j = 0; j < col_two; j++){
              fprintf(final,"%f ",answer[i][j]);
            }
            fprintf(final,"\n");
          }
          //Close the file when done
          fclose(final);
        }

        //This will run if the dimensions of the matrices are incorrect, meaning they cant be multiplied
        //So we print an error message for the user. Also I belive we have to make the slaves stop running
        //so we use that MPI_Send message to them to tell them to stop
        else{
          printf("\nInconceivable! We cannot multiply these matrices buddy!\n");
          //Iterate through each slave and tell them to stop
          for(i = 0; i < numprocs - 1; i++){
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, i+1, 0, MPI_COMM_WORLD);
          }
        }
      }else {

        //So this is where the Slave code should go!
        //The first thing that we want to do is recv the stuff we sent.
        MPI_Recv(size, 3, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        //We check the tag and if it is 0 we know that they aren't compatable
        if(status.MPI_TAG==0){
          compatable = false;
        }

        //We set some variables to some stuff and make compatable true again
        else{
          compatable = true;
          nrows=size[0];
          ncols=size[1];
          iteration=size[2];

          //Then we allocate some memory for these guys
          b = (double*)malloc(ncols * sizeof(double));
          buffer = (double*)malloc(ncols * sizeof(double));
        }

        //This is the slave code that we got from the slides (slide 71)
        if(compatable){
          for(i = 0; i < iteration; i++){
            MPI_Bcast(b, ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            if (myid <= nrows) {
              while(1) {
                MPI_Recv(buffer, ncols, MPI_DOUBLE, 0, MPI_ANY_TAG,
                MPI_COMM_WORLD, &status);
                if (status.MPI_TAG == 0){
                  break;
                }
                row = status.MPI_TAG;
                ans = 0.0;
                for (j = 0; j < ncols; j++) {
                  ans += buffer[j] * b[j];
                }
                MPI_Send(&ans, 1, MPI_DOUBLE, 0, row, MPI_COMM_WORLD);
              }
            }
          }
        }
      }
    }
    //If they didn't put in enough arguments to run the file
    else{
      if(myid==0){
	      fprintf(stderr, "Need the correct number of arguements 3. So use a.txt and b.txt\n");
	    }
    }

    //We need to run the finalize at the very end of the program because thats what the
    //slides said to do
    MPI_Finalize();
    return 0;

}

