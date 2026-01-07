-- Migration: Add get_room_state RPC function
-- Purpose: Allow clients to pull room snapshot (rules + members) from DB
-- Usage: Fix reload page and seed data issues

CREATE OR REPLACE FUNCTION public.get_room_state(
    p_room_id INTEGER
)
RETURNS json
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
DECLARE
    v_room_data json;
    v_members_data json;
BEGIN
    -- Get room rules
    SELECT json_build_object(
        'mode', mode,
        'maxPlayers', max_players,
        'wagerMode', wager_mode,
        'visibility', visibility
    ) INTO v_room_data
    FROM public.rooms
    WHERE id = p_room_id;

    -- Get room members
    SELECT json_agg(
        json_build_object(
            'account_id', rm.account_id,
            'username', COALESCE(p.name, a.email),
            'is_host', (r.host_id = rm.account_id),
            'is_ready', false
        )
    ) INTO v_members_data
    FROM public.room_members rm
    JOIN public.accounts a ON rm.account_id = a.id
    LEFT JOIN public.profiles p ON rm.account_id = p.account_id
    JOIN public.rooms r ON rm.room_id = r.id
    WHERE rm.room_id = p_room_id;

    -- Return combined result
    RETURN json_build_object(
        'rules', v_room_data,
        'players', COALESCE(v_members_data, '[]'::json)
    );
END;
$$;

-- Grant permission
GRANT EXECUTE ON FUNCTION public.get_room_state(INTEGER) TO service_role;
