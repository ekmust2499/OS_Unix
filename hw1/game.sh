#!/bin/bash

trap "cleanup" SIGTERM SIGKILL SIGINT

function cleanup() {
  rm -f $fifo 2>/dev/null
  exit
}

function init() {
  fifo=game_fifo
  clear 2>/dev/null
  
  if [[ ! -p $fifo ]]
  then
    mknod $fifo p
	is_first_player=1
    symbol='X'
    enemy_symbol='O'
  else
	is_first_player=0
    symbol='O'
    enemy_symbol='X'
  fi
}

game_map=()
  for ((i=0; i<9; i++)) 
  do
    game_map[$i]=" "
  done

function draw_game_map() {
  clear
  echo " ${game_map[0]} | ${game_map[1]} | ${game_map[2]} "
  echo "---+---+---"
  echo " ${game_map[3]} | ${game_map[4]} | ${game_map[5]} "
  echo "---+---+---"
  echo " ${game_map[6]} | ${game_map[7]} | ${game_map[8]} "
}

function set_game_map() {
  i=$((X * 3 + Y))
  game_map[$i]=$1
}

function read_pos() {
  while true
  do
    local str
    if [[ -z $1 ]]
    then
      echo -n "Твой ход: "
	  read -r str 
	else 
	  read -r str < "$1"
    fi
	
	if [[ $str =~ ^([1-3])\ ([1-3])$ ]] 
    then
      X="${BASH_REMATCH[1]}"
      Y="${BASH_REMATCH[2]}"
    else
      echo "Неверный ввод данных. Формат: x y (от 1 до 3)."
      continue
    fi

    X=$((X-1))
    Y=$((Y-1))
    i=$((X * 3 + Y))

    if [[ ${game_map[$i]} != " " ]]
    then
      echo "Позиция занята. Попробуй ещё раз." 
    else
      break
    fi
  done
}

function game_loop() {
  if [[ "$is_first_player" -eq 1 ]]
  then
    #ходит 1й игрок (кто начал игру)
    draw_game_map
    read_pos
    echo "$((X+1)) $((Y+1))" > $fifo
    set_game_map $symbol
  fi

  while true
  do
    draw_game_map
    echo "Идёт ход противника..."
	#ходит 2й игрок
    read_pos $fifo
    set_game_map $enemy_symbol
    check_end_game $enemy_symbol
    draw_game_map
	#ходит 1й игрок
    read_pos
    echo "$((X+1)) $((Y+1))" > $fifo
    set_game_map $symbol
    check_end_game $symbol
  done

}

function check_end_game() {
  if check_row "$1" || check_column "$1" || check_diagonals "$1"
  then
    game_over 0 "$1"
  fi

  if check_empty_cells
  then
    game_over 1
  fi
}

function game_over() {
  draw_game_map

  if [[ $1 -eq 1 ]]
  then
    echo "Ничья!"
    cleanup
  fi

  if [[ $2 == "$symbol" ]]
  then
    echo "Победа!"
  else
    echo "Проигрыш!"
  fi

  cleanup
}

function check_empty_cells() {
  is_filled=0;
  for ((i=0; i<9; i++)) 
  do
    if [[ ${game_map[$i]} == " " ]]
	then
	  is_filled=1
	fi
  done
  return $is_filled
}

function check_row() {
  if [[ ${game_map[0]} == "$1" ]] && [[ ${game_map[1]} == "$1" ]] && [[ ${game_map[2]} == "$1" ]]
  then
    return 0
  fi

  if [[ ${game_map[3]} == "$1" ]] && [[ ${game_map[4]} == "$1" ]] && [[ ${game_map[5]} == "$1" ]]
  then
    return 0
  fi
  
  if [[ ${game_map[6]} == "$1" ]] && [[ ${game_map[7]} == "$1" ]] && [[ ${game_map[8]} == "$1" ]]
  then
    return 0
  fi

  return 1
}

function check_column() {
  if [[ ${game_map[0]} == "$1" ]] && [[ ${game_map[3]} == "$1" ]] && [[ ${game_map[6]} == "$1" ]]
  then
    return 0
  fi

  if [[ ${game_map[1]} == "$1" ]] && [[ ${game_map[4]} == "$1" ]] && [[ ${game_map[7]} == "$1" ]]
  then
    return 0
  fi
  
  if [[ ${game_map[2]} == "$1" ]] && [[ ${game_map[5]} == "$1" ]] && [[ ${game_map[8]} == "$1" ]]
  then
    return 0
  fi

  return 1
}

function check_diagonals() {
  if [[ ${game_map[4]} == "$1" ]] && [[ ${game_map[0]} == "$1" ]] && [[ ${game_map[8]} == "$1" ]]
  then
    return 0
  fi

  if [[ ${game_map[4]} == "$1" ]] && [[ ${game_map[2]} == "$1" ]] && [[ ${game_map[6]} == "$1" ]]
  then
    return 0
  fi

  return 1
}

init
game_loop