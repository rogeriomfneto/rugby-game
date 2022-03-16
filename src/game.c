// Standard headers
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Internal headers
#include "field.h"
#include "spy.h"

// Main header
#include "game.h"

// Macros
#define UNUSED(x) (void)(x) // Auxiliary to avoid error of unused parameter

/*----------------------------------------------------------------------------*/
/*                        PRIVATE STRUCT IMPLEMENTATION                       */
/*----------------------------------------------------------------------------*/

struct game {
  Field field;

  size_t max_number_spies;

  PlayerStrategy execute_attacker_strategy;
  PlayerStrategy execute_defender_strategy;

  Item attacker;
  Item defender;
  Item obstacle;

  Spy attacker_spy;
  Spy defender_spy;
};

/*----------------------------------------------------------------------------*/
/*                          PRIVATE FUNCTIONS HEADERS                         */
/*----------------------------------------------------------------------------*/

Game allocate_game(
  dimension_t field_dimension,
  size_t max_number_spies,
  PlayerStrategy execute_attacker_strategy,
  PlayerStrategy execute_defender_strategy);

void set_attacker_in_field(Field field, Item attacker);
void set_defender_in_field(Field field, Item defender);
void set_obstacles_in_field(Field field, Item obstacle);

bool has_spy_exceeded_max_number_uses(Spy opponent_spy,
                                      size_t max_number_spies);
bool has_defender_captured_attacker(Item defender, Item attacker);
bool has_attacker_arrived_end_field(Field field, Item attacker);

void move_item(Field field,
               Item item,
               Spy opponent_spy,
               PlayerStrategy execute_item_strategy);

/*----------------------------------------------------------------------------*/
/*                              PUBLIC FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

Game new_game(
    dimension_t field_dimension,
    size_t max_number_spies,
    PlayerStrategy execute_attacker_strategy,
    PlayerStrategy execute_defender_strategy) {
  Game game = allocate_game(
      field_dimension,
      max_number_spies,
      execute_attacker_strategy,
      execute_defender_strategy);

  set_attacker_in_field(game->field, game->attacker);
  set_defender_in_field(game->field, game->defender);
  set_obstacles_in_field(game->field, game->obstacle);

  return game;
}

/*----------------------------------------------------------------------------*/

void delete_game(Game game) {
  if (game == NULL) return;

  delete_spy(game->defender_spy);
  game->defender_spy = NULL;

  delete_spy(game->attacker_spy);
  game->attacker_spy = NULL;

  delete_item(game->obstacle);
  game->obstacle = NULL;

  delete_item(game->defender);
  game->defender = NULL;

  delete_item(game->attacker);
  game->attacker = NULL;

  game->execute_defender_strategy = NULL;
  game->execute_attacker_strategy = NULL;

  game->max_number_spies = 0;

  delete_field(game->field);
  game->field = NULL;

  free(game);
}

/*----------------------------------------------------------------------------*/

void print_game(Game game) {
  if (game == NULL) return;

  print_field_grid(game->field);
}

/*----------------------------------------------------------------------------*/

void play_game(Game game, size_t max_turns) {
  if (game == NULL) return;

  printf("Turn 0\n");
  print_game(game);

  for (size_t turn = 0; turn < max_turns; turn++) {
    printf("Turn %ld\n", turn+1);

    move_item(game->field,
              game->attacker,
              game->defender_spy,
              game->execute_attacker_strategy);

    move_item(game->field,
              game->defender,
              game->attacker_spy,
              game->execute_defender_strategy);

    print_game(game);

    if (has_spy_exceeded_max_number_uses(
          game->defender_spy, game->max_number_spies)) {
      printf("GAME OVER! Attacker cheated spying more than %ld %s!\n",
             game->max_number_spies,
             game->max_number_spies == 1UL ? "time" : "times");
      return;
    }

    if (has_spy_exceeded_max_number_uses(
          game->attacker_spy, game->max_number_spies)) {
      printf("GAME OVER! Defender cheated spying more than %ld %s!\n",
             game->max_number_spies,
             game->max_number_spies == 1UL ? "time" : "times");
      return;
    }

    if (has_attacker_arrived_end_field(game->field, game->attacker)) {
      printf("GAME OVER! Attacker wins!\n");
      return;
    }

    if (has_defender_captured_attacker(game->attacker, game->defender)) {
      printf("GAME OVER! Defender wins!\n");
      return;
    }
  }

  // A draw happens only if nobody wins before max_turns
  printf("GAME OVER! Attacker and Defender draw!\n");
}

/*----------------------------------------------------------------------------*/
/*                             PRIVATE FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

Game allocate_game(
    dimension_t field_dimension,
    size_t max_number_spies,
    PlayerStrategy execute_attacker_strategy,
    PlayerStrategy execute_defender_strategy) {
  Game game = malloc(sizeof(*game));

  game->field = new_field(field_dimension);

  game->max_number_spies = max_number_spies;

  game->execute_attacker_strategy = execute_attacker_strategy;
  game->execute_defender_strategy = execute_defender_strategy;

  game->attacker = new_item('A', true);
  game->defender = new_item('D', true);
  game->obstacle = new_item('X', false);

  game->attacker_spy = new_spy(game->attacker);
  game->defender_spy = new_spy(game->defender);

  return game;
}

/*----------------------------------------------------------------------------*/

void set_attacker_in_field(Field field, Item attacker) {
  if (field == NULL || attacker == NULL) return;

  dimension_t field_dimension = get_field_dimension(field);

  position_t attacker_initial_position = {
    field_dimension.height / 2, 1 // Left side of field
  };
  add_item_to_field(field, attacker, attacker_initial_position);
}

/*----------------------------------------------------------------------------*/

void set_defender_in_field(Field field, Item defender) {
  if (field == NULL || defender == NULL) return;

  dimension_t field_dimension = get_field_dimension(field);

  position_t defender_initial_position = {
    field_dimension.height / 2, field_dimension.width-2 // Right side of field
  };
  add_item_to_field(field, defender, defender_initial_position);
}

/*----------------------------------------------------------------------------*/

void set_obstacles_in_field(Field field, Item obstacle) {
  dimension_t field_dimension = get_field_dimension(field);
  size_t field_height = field_dimension.height;
  size_t field_width = field_dimension.width;

  for (size_t i = 0; i < field_height; i++) {
    position_t left_border = { i, 0 };
    add_item_to_field(field, obstacle, left_border);
  }

  for (size_t i = 0; i < field_height; i++) {
    position_t right_border = { i, field_width-1 };
    add_item_to_field(field, obstacle, right_border);
  }

  for (size_t j = 0; j < field_width; j++) {
    position_t top_border = { 0, j };
    add_item_to_field(field, obstacle, top_border);
  }

  for (size_t j = 0; j < field_width; j++) {
    position_t bottom_border = { field_height-1, j };
    add_item_to_field(field, obstacle, bottom_border);
  }
}

/*----------------------------------------------------------------------------*/

bool has_spy_exceeded_max_number_uses(Spy opponent_spy,
                                      size_t max_number_spies) {
  return get_spy_number_uses(opponent_spy) > max_number_spies;
}

/*----------------------------------------------------------------------------*/

bool has_defender_captured_attacker(Item defender, Item attacker) {
  if (attacker == NULL || defender == NULL) return false;

  position_t attacker_position = get_item_position(attacker);
  position_t defender_position = get_item_position(defender);

  return neighbor_positions(defender_position, attacker_position);
}

/*----------------------------------------------------------------------------*/

bool has_attacker_arrived_end_field(Field field, Item attacker) {
  if (field == NULL || attacker == NULL) return false;

  position_t attacker_position = get_item_position(attacker);
  dimension_t field_dimension = get_field_dimension(field);

  return attacker_position.j == field_dimension.width - 1;
}

/*----------------------------------------------------------------------------*/

void move_item(Field field,
               Item item,
               Spy opponent_spy,
               PlayerStrategy execute_item_strategy) {
  position_t item_position = get_item_position(item);

  direction_t item_direction
    = execute_item_strategy(item_position, opponent_spy);

  move_item_in_field(field, item, item_direction);
}

/*----------------------------------------------------------------------------*/
