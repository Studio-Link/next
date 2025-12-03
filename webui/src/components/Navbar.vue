<template>
	<nav class="text-sl-primary flex justify-between h-12" aria-label="Navigation">
		<div class="flex">
			<RouterLink to="/" activeClass="text-sl-primary"
				class="inline-flex items-center px-1 pt-1 text-base font-medium leading-5 text-sl-disabled hover:text-sl-on_surface_1"
			>
				<svg class="w-5 h-5 mr-1" viewBox="0 0 20 20" fill="currentColor" aria-hidden="true">
					<path
						d="M5 3a2 2 0 00-2 2v2a2 2 0 002 2h2a2 2 0 002-2V5a2 2 0 00-2-2H5zM5 11a2 2 0 00-2 2v2a2 2 0 002 2h2a2 2 0 002-2v-2a2 2 0 00-2-2H5zM11 5a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V5zM11 13a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z"
					/>
				</svg> Tracks
			</RouterLink>
			<RouterLink to="/debug" activeClass="text-sl-primary"
				href="#"
				class="ml-4 inline-flex items-center px-1 pt-1 border-transparent text-base font-medium leading-5 text-sl-disabled hover:text-sl-on_surface_1"
			>Options</RouterLink>
		</div>
	</nav>
</template>
