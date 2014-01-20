      SUBROUTINE LOADINI
!
! WRITTEN BY YOUSU CHEN, PNNL, 03 JAN 2013
!
!------------DESCRIPTION------------------
! LOAD MODEL INITIAL INDICE
! 
!
      USE BUSMODULE, ONLY: BUS_I, BUS_I_SAVE
      USE GENMODULE, ONLY: GEN_STATUS, GEN_STATUS_SAVE
      USE BRCHMODULE
      USE MPI
      USE DEFDP

      IMPLICIT NONE

      BUS_I=BUS_I_SAVE
      F_BUS=F_BUS_SAVE
      T_BUS=T_BUS_SAVE
      GEN_STATUS=GEN_STATUS_SAVE
      BR_STATUS=BR_STATUS_SAVE

      ENDSUBROUTINE