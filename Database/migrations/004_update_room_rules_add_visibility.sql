-- Drop old function (4 params)
DROP FUNCTION IF EXISTS public.update_room_rules(integer, character varying, integer, boolean);

-- Drop if 5-param version exists
DROP FUNCTION IF EXISTS public.update_room_rules(integer, character varying, integer, boolean, character varying);

-- Create new function with visibility parameter (RETURNS json)
CREATE OR REPLACE FUNCTION public.update_room_rules(
  p_room_id integer,
  p_mode character varying,
  p_max_players integer,
  p_wager_mode boolean,
  p_visibility character varying
)
RETURNS json
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
  UPDATE public.rooms
  SET
    mode = p_mode,
    max_players = p_max_players,
    wager_mode = p_wager_mode,
    visibility = p_visibility,
    updated_at = now()
  WHERE id = p_room_id;

  RETURN json_build_object(
    'success', true,
    'room_id', p_room_id,
    'mode', p_mode,
    'max_players', p_max_players,
    'wager_mode', p_wager_mode,
    'visibility', p_visibility
  );
END;
$$;

-- Grant permission to service_role
GRANT EXECUTE ON FUNCTION public.update_room_rules(
  integer, character varying, integer, boolean, character varying
) TO service_role;
